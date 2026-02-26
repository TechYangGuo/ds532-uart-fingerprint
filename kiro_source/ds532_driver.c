/*
 * DS532指纹模块UART驱动 - 源文件
 * 
 * 版权所有 (C) 2026
 * 作者: Kiro开发团队
 * 
 * 本驱动实现DS532方形指纹模块的Linux UART驱动
 * 平台: NXP IMX6ULL
 * 内核: Linux 4.9.88
 * 接口: UART6 (/dev/ttymxc5)
 * 
 * 第1阶段实现：最小可运行驱动框架
 * 第2阶段实现：DS532协议封装
 * 第3阶段实现：UART通信
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/string.h>
#include <linux/tty.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/termios.h>
#include <linux/tty_ldisc.h>
#include <linux/file.h>
#include <linux/time.h>
#include <linux/printk.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>

#include "ds532_driver.h"

/* 设备名称和类名 */
#define DEVICE_NAME	"ds532_fp"
#define CLASS_NAME	"ds532"

/* TTY设备路径 */
#define TTY_DEVICE_PATH	"/dev/ttymxc5"

/* 全局设备指针 */
static struct ds532_dev *g_ds532_dev;

/*
 * ============================================================================
 * 静态函数前置声明
 * ============================================================================
 */

/* TTY通信函数 */
static int ds532_tty_open(struct ds532_dev *dev);
static void ds532_tty_close(struct ds532_dev *dev);
static int ds532_send_packet(struct ds532_dev *dev, const u8 *buffer, size_t len);
static int ds532_recv_packet(struct ds532_dev *dev, u8 *buffer, size_t max_len);

/* GPIO中断处理函数 */
static irqreturn_t ds532_touch_irq_handler(int irq, void *dev_id);
static int ds532_gpio_init(struct ds532_dev *dev);
static void ds532_gpio_cleanup(struct ds532_dev *dev);

/* 字符设备文件操作函数 */
static int ds532_open(struct inode *inode, struct file *filp);
static int ds532_release(struct inode *inode, struct file *filp);
static ssize_t ds532_read(struct file *filp, char __user *buf, size_t count, loff_t *offset);
static ssize_t ds532_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset);
static long ds532_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

/* Platform驱动函数 */
static int ds532_probe(struct platform_device *pdev);
static int ds532_remove(struct platform_device *pdev);

/* sysfs属性显示函数 */
static ssize_t touch_count_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t last_touch_time_show(struct device *dev, struct device_attribute *attr, char *buf);

/* sysfs属性组前置声明 */
static const struct attribute_group ds532_attr_group;

/* 
 * 注意：静态函数前置声明已在文件开头完成
 * 这里不需要重复声明
 */

/*
 * ============================================================================
 * 字符设备文件操作函数
 * ============================================================================
 */

/**
* 
*/


/**
 * ds532_open - 打开设备
 * @inode: inode结构体指针
 * @filp: file结构体指针
 * 
 * 实现设备独占访问控制：同一时刻只允许一个进程打开设备
 * 
 * 返回值：
 *   0 - 成功
 *   -EBUSY - 设备已被占用
 */
static int ds532_open(struct inode *inode, struct file *filp)
{
	struct ds532_dev *dev = g_ds532_dev;
	
	pr_info("[DS532] Device open attempt\n");
	
	/* 检查设备是否已被打开 */
	if (atomic_read(&dev->device_opened)) {
		pr_warn("[DS532] Device already opened\n");
		return -EBUSY;
	}
	
	/* 原子地设置设备打开标志 */
	if (atomic_cmpxchg(&dev->device_opened, 0, 1) != 0) {
		pr_warn("[DS532] Device opened by another process\n");
		return -EBUSY;
	}
	
	/* 保存设备指针到文件私有数据 */
	filp->private_data = dev;
	
	pr_info("[DS532] Device opened successfully\n");
	return 0;
}

/**
 * ds532_release - 关闭设备
 * @inode: inode结构体指针
 * @filp: file结构体指针
 * 
 * 释放设备独占访问标志
 * 
 * 返回值：0 - 成功
 */
static int ds532_release(struct inode *inode, struct file *filp)
{
	struct ds532_dev *dev = filp->private_data;
	
	/* 清除设备打开标志 */
	atomic_set(&dev->device_opened, 0);
	
	pr_info("[DS532] Device closed\n");
	return 0;
}

/**
 * ds532_read - 读取设备数据
 * @filp: file结构体指针
 * @buf: 用户空间缓冲区
 * @count: 请求读取的字节数
 * @offset: 文件偏移（未使用）
 * 
 * 读取最后一次接收到的响应数据
 * 
 * 返回值：
 *   >0 - 实际读取的字节数
 *   0 - 无数据可读
 *   <0 - 错误码
 */
static ssize_t ds532_read(struct file *filp, char __user *buf,
			  size_t count, loff_t *offset)
{
	struct ds532_dev *dev = filp->private_data;
	ssize_t ret = 0;
	
	pr_debug("[DS532] Read request: %zu bytes\n", count);
	
	/* 获取互斥锁 */
	if (mutex_lock_interruptible(&dev->io_mutex))
		return -ERESTARTSYS;
	
	/* 从UART接收数据（Stage 3） */
	ret = ds532_recv_packet(dev, dev->rx_buffer, DS532_BUFFER_SIZE);
	if (ret < 0) {
		pr_err("[DS532] Failed to receive packet: %d\n", (int)ret);
		goto out;
	}
	
	dev->rx_len = ret;
	
	/* 限制读取长度 */
	if (count > dev->rx_len)
		count = dev->rx_len;
	
	/* 复制数据到用户空间 */
	if (copy_to_user(buf, dev->rx_buffer, count)) {
		pr_err("[DS532] Failed to copy data to user space\n");
		ret = -EFAULT;
		goto out;
	}
	
	ret = count;
	pr_debug("[DS532] Read %zd bytes\n", ret);
	
out:
	mutex_unlock(&dev->io_mutex);
	return ret;
}

/**
 * ds532_write - 写入设备数据
 * @filp: file结构体指针
 * @buf: 用户空间缓冲区
 * @count: 要写入的字节数
 * @offset: 文件偏移（未使用）
 * 
 * 发送命令数据到指纹模块
 * 
 * 返回值：
 *   >0 - 实际写入的字节数
 *   <0 - 错误码
 */
static ssize_t ds532_write(struct file *filp, const char __user *buf,
			   size_t count, loff_t *offset)
{
	struct ds532_dev *dev = filp->private_data;
	ssize_t ret;
	
	pr_debug("[DS532] Write request: %zu bytes\n", count);
	
	/* 检查写入长度 */
	if (count > DS532_BUFFER_SIZE) {
		pr_err("[DS532] Write size too large: %zu\n", count);
		return -EINVAL;
	}
	
	/* 获取互斥锁 */
	if (mutex_lock_interruptible(&dev->io_mutex))
		return -ERESTARTSYS;
	
	/* 从用户空间复制数据 */
	if (copy_from_user(dev->tx_buffer, buf, count)) {
		pr_err("[DS532] Failed to copy data from user space\n");
		ret = -EFAULT;
		goto out;
	}
	
	/* 发送数据到UART（Stage 3） */
	ret = ds532_send_packet(dev, dev->tx_buffer, count);
	if (ret < 0) {
		pr_err("[DS532] Failed to send packet: %d\n", (int)ret);
		goto out;
	}
	
	pr_debug("[DS532] Write %zd bytes\n", ret);
	
out:
	mutex_unlock(&dev->io_mutex);
	return ret;
}

/**
 * ds532_ioctl - 设备控制操作
 * @filp: file结构体指针
 * @cmd: 命令码
 * @arg: 命令参数
 * 
 * 执行DS532协议命令
 * 
 * 返回值：
 *   0 - 成功
 *   <0 - 错误码
 */
static long ds532_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct ds532_dev *dev = filp->private_data;
	int ret = 0;
	
	pr_debug("[DS532] IOCTL command: 0x%x\n", cmd);
	
	/* 验证命令码 */
	if (_IOC_TYPE(cmd) != DS532_IOC_MAGIC) {
		pr_err("[DS532] Invalid IOCTL magic: 0x%x\n", _IOC_TYPE(cmd));
		return -ENOTTY;
	}
	
	/* 获取互斥锁 */
	if (mutex_lock_interruptible(&dev->io_mutex))
		return -ERESTARTSYS;
	
	/* 处理命令 */
	switch (cmd) {
	case DS532_IOC_INIT:
		pr_info("[DS532] IOCTL: Init (VfyPwd)\n");
		/* TODO: 第2阶段实现协议封装 */
		ret = 0;
		break;
		
	case DS532_IOC_GETIMAGE:
		pr_info("[DS532] IOCTL: GetImage\n");
		/* TODO: 第2阶段实现协议封装 */
		ret = 0;
		break;
		
	case DS532_IOC_GENCHAR:
		pr_info("[DS532] IOCTL: GenChar\n");
		/* TODO: 第2阶段实现协议封装 */
		ret = 0;
		break;
		
	case DS532_IOC_MATCH:
		pr_info("[DS532] IOCTL: Match\n");
		/* TODO: 第2阶段实现协议封装 */
		ret = 0;
		break;
		
	case DS532_IOC_SEARCH:
		pr_info("[DS532] IOCTL: Search\n");
		/* TODO: 第2阶段实现协议封装 */
		ret = 0;
		break;
		
	case DS532_IOC_STORE:
		pr_info("[DS532] IOCTL: Store\n");
		/* TODO: 第2阶段实现协议封装 */
		ret = 0;
		break;
		
	case DS532_IOC_DELETE:
		pr_info("[DS532] IOCTL: Delete\n");
		/* TODO: 第2阶段实现协议封装 */
		ret = 0;
		break;
		
	case DS532_IOC_EMPTY:
		pr_info("[DS532] IOCTL: Empty\n");
		/* TODO: 第2阶段实现协议封装 */
		ret = 0;
		break;
		
	default:
		pr_err("[DS532] Unknown IOCTL command: 0x%x\n", cmd);
		ret = -ENOTTY;
		break;
	}
	
	mutex_unlock(&dev->io_mutex);
	return ret;
}

/* 文件操作结构体 */
static const struct file_operations ds532_fops = {
	.owner		= THIS_MODULE,
	.open		= ds532_open,
	.release	= ds532_release,
	.read		= ds532_read,
	.write		= ds532_write,
	.unlocked_ioctl	= ds532_ioctl,
};

/*
 * ============================================================================
 * Platform驱动函数
 * ============================================================================
 */

/**
 * ds532_probe - 驱动探测函数
 * @pdev: platform设备指针
 * 
 * 当设备树中的compatible匹配时被调用
 * 负责初始化设备、分配资源、注册字符设备
 * 
 * 返回值：
 *   0 - 成功
 *   <0 - 错误码
 */
static int ds532_probe(struct platform_device *pdev)
{
	struct ds532_dev *dev;
	int ret;
	
	pr_info("[DS532] Probing DS532 driver\n");
	
	/* 分配设备私有数据 */
	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		pr_err("[DS532] Failed to allocate device memory\n");
		return -ENOMEM;
	}
	
	/* 保存平台设备指针 */
	dev->pdev = pdev;
	platform_set_drvdata(pdev, dev);
	g_ds532_dev = dev;
	
	/* 初始化互斥锁和原子变量 */
	mutex_init(&dev->io_mutex);
	atomic_set(&dev->device_opened, 0);
	
	/* TODO: 第1阶段 - 解析设备树参数 */
	/* 从设备树读取波特率等配置 */
	ret = of_property_read_u32(pdev->dev.of_node, "baudrate", &dev->baudrate);
	if (ret) {
		dev->baudrate = 57600;  /* 默认波特率 */
		pr_info("[DS532] Using default baudrate: %u\n", dev->baudrate);
	} else {
		pr_info("[DS532] Baudrate from DT: %u\n", dev->baudrate);
	}
	
	/* 分配字符设备号 */
	ret = alloc_chrdev_region(&dev->devno, 0, 1, DEVICE_NAME);
	if (ret < 0) {
		pr_err("[DS532] Failed to allocate chrdev region: %d\n", ret);
		goto err_alloc_chrdev;
	}
	pr_info("[DS532] Allocated device number: %d:%d\n",
		MAJOR(dev->devno), MINOR(dev->devno));
	
	/* 初始化字符设备 */
	cdev_init(&dev->cdev, &ds532_fops);
	dev->cdev.owner = THIS_MODULE;
	
	/* 添加字符设备 */
	ret = cdev_add(&dev->cdev, dev->devno, 1);
	if (ret < 0) {
		pr_err("[DS532] Failed to add cdev: %d\n", ret);
		goto err_cdev_add;
	}
	
	/* 创建设备类 */
	dev->class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(dev->class)) {
		ret = PTR_ERR(dev->class);
		pr_err("[DS532] Failed to create class: %d\n", ret);
		goto err_class_create;
	}
	
	/* 创建设备节点 */
	dev->device = device_create(dev->class, NULL, dev->devno,
				    dev, DEVICE_NAME);
	if (IS_ERR(dev->device)) {
		ret = PTR_ERR(dev->device);
		pr_err("[DS532] Failed to create device: %d\n", ret);
		goto err_device_create;
	}
	
	/* 创建sysfs属性（Stage 4） */
	ret = sysfs_create_group(&dev->device->kobj, &ds532_attr_group);
	if (ret < 0) {
		pr_err("[DS532] Failed to create sysfs attributes: %d\n", ret);
		goto err_sysfs_create;
	}
	
	/* 打开并配置TTY设备（Stage 3） */
	ret = ds532_tty_open(dev);
	if (ret < 0) {
		pr_err("[DS532] Failed to open TTY device: %d\n", ret);
		goto err_tty_open;
	}
	
	/* 初始化GPIO中断（Stage 4） */
	ret = ds532_gpio_init(dev);
	if (ret < 0) {
		pr_err("[DS532] Failed to initialize GPIO: %d\n", ret);
		goto err_gpio_init;
	}
	
	pr_info("[DS532] Driver initialized successfully\n");
	pr_info("[DS532] Device node: /dev/%s\n", DEVICE_NAME);
	return 0;

err_gpio_init:
	sysfs_remove_group(&dev->device->kobj, &ds532_attr_group);
err_sysfs_create:
	ds532_tty_close(dev);
err_tty_open:
	device_destroy(dev->class, dev->devno);
err_device_create:
	class_destroy(dev->class);
err_class_create:
	cdev_del(&dev->cdev);
err_cdev_add:
	unregister_chrdev_region(dev->devno, 1);
err_alloc_chrdev:
	return ret;
}

/**
 * ds532_remove - 驱动移除函数
 * @pdev: platform设备指针
 * 
 * 驱动卸载时被调用
 * 负责释放所有资源
 * 
 * 返回值：0 - 成功
 */
static int ds532_remove(struct platform_device *pdev)
{
	struct ds532_dev *dev = platform_get_drvdata(pdev);
	
	pr_info("[DS532] Removing DS532 driver\n");
	
	/* 清理GPIO中断（Stage 4） */
	ds532_gpio_cleanup(dev);
	
	/* 移除sysfs属性（Stage 4） */
	sysfs_remove_group(&dev->device->kobj, &ds532_attr_group);
	
	/* 关闭TTY设备（Stage 3） */
	ds532_tty_close(dev);
	
	/* 销毁设备节点 */
	device_destroy(dev->class, dev->devno);
	
	/* 销毁设备类 */
	class_destroy(dev->class);
	
	/* 删除字符设备 */
	cdev_del(&dev->cdev);
	
	/* 释放设备号 */
	unregister_chrdev_region(dev->devno, 1);
	
	pr_info("[DS532] Driver removed successfully\n");

	return 0;
}

/* 设备树匹配表 */
static const struct of_device_id ds532_of_match[] = {
	{ .compatible = "ds532-uart" },
	{ }
};
MODULE_DEVICE_TABLE(of, ds532_of_match);

/* Platform驱动结构体 */
static struct platform_driver ds532_driver = {
	.driver = {
		.name		= "ds532_driver",
		.of_match_table	= ds532_of_match,
	},
	.probe	= ds532_probe,
	.remove	= ds532_remove,
};

/* 注册Platform驱动 */
module_platform_driver(ds532_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kiro Development Team");
MODULE_DESCRIPTION("DS532 Fingerprint UART Driver for IMX6ULL");
MODULE_VERSION("0.4-stage4");

/*
 * ============================================================================
 * UART通信函数实现（Stage 3）- 简化版本适配Linux 4.9.88
 * ============================================================================
 */

/**
 * ds532_tty_open - 打开并配置TTY设备
 * @dev: 设备私有数据指针
 * 
 * 简化版本：仅打开TTY设备，配置由用户空间完成
 * 
 * 返回值：
 *   0 - 成功
 *   <0 - 错误码
 */
static int ds532_tty_open(struct ds532_dev *dev)
{
	struct file *tty_file;
	
	pr_info("[DS532] Opening TTY device: %s\n", TTY_DEVICE_PATH);
	
	/* 打开TTY设备文件 */
	tty_file = filp_open(TTY_DEVICE_PATH, O_RDWR | O_NOCTTY, 0);
	if (IS_ERR(tty_file)) {
		pr_err("[DS532] Failed to open TTY device: %ld\n", PTR_ERR(tty_file));
		return PTR_ERR(tty_file);
	}
	
	/* 保存TTY文件指针 */
	dev->tty_file = tty_file;
	dev->tty = NULL;  /* 简化版本不直接操作tty_struct */
	
	pr_info("[DS532] TTY device opened successfully\n");
	pr_info("[DS532] Note: Please configure TTY manually: stty -F %s 57600 cs8 -cstopb -parenb raw\n", 
		TTY_DEVICE_PATH);
	
	return 0;
}

/**
 * ds532_tty_close - 关闭TTY设备
 * @dev: 设备私有数据指针
 */
static void ds532_tty_close(struct ds532_dev *dev)
{
	if (dev->tty_file) {
		pr_info("[DS532] Closing TTY device\n");
		filp_close(dev->tty_file, NULL);
		dev->tty_file = NULL;
		dev->tty = NULL;
	}
}

/**
 * ds532_send_packet - 发送数据包到UART
 * @dev: 设备私有数据指针
 * @buffer: 数据包缓冲区
 * @len: 数据包长度
 * 
 * 通过TTY文件发送数据包，超时时间500ms
 * 
 * 返回值：
 *   >0 - 实际发送的字节数
 *   <0 - 错误码
 */
static int ds532_send_packet(struct ds532_dev *dev, const u8 *buffer, size_t len)
{
	struct file *tty_file = dev->tty_file;
	mm_segment_t old_fs;
	loff_t pos = 0;
	int ret;
	unsigned long timeout;
	size_t sent = 0;
	
	if (!tty_file) {
		pr_err("[DS532] TTY not ready for write\n");
		return -ENODEV;
	}
	
	pr_debug("[DS532] Sending packet: %zu bytes\n", len);
	
	/* 打印发送的数据包（调试用） */
	if (len <= 32) {
		print_hex_dump(KERN_DEBUG, "[DS532] TX: ", DUMP_PREFIX_NONE,
			       16, 1, buffer, len, true);
	}
	
	/* 设置内核空间访问 */
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	
	/* 设置超时时间 */
	timeout = jiffies + msecs_to_jiffies(DS532_SEND_TIMEOUT_MS);
	
	/* 发送数据 */
	while (sent < len) {
		ret = vfs_write(tty_file, buffer + sent, len - sent, &pos);
		if (ret < 0) {
			pr_err("[DS532] TTY write error: %d\n", ret);
			set_fs(old_fs);
			return ret;
		}
		
		sent += ret;
		
		/* 检查超时 */
		if (time_after(jiffies, timeout)) {
			pr_err("[DS532] Send timeout: sent %zu/%zu bytes\n", sent, len);
			set_fs(old_fs);
			return -ETIMEDOUT;
		}
		
		/* 如果没有发送完，短暂延时后重试 */
		if (sent < len)
			msleep(10);
	}
	
	set_fs(old_fs);
	
	pr_debug("[DS532] Packet sent: %zu bytes\n", sent);
	return sent;
}

/**
 * ds532_recv_packet - 从UART接收数据包
 * @dev: 设备私有数据指针
 * @buffer: 接收缓冲区
 * @max_len: 缓冲区最大长度
 * 
 * 从TTY文件接收数据包，超时时间2000ms
 * 
 * 返回值：
 *   >0 - 实际接收的字节数
 *   <0 - 错误码
 */
static int ds532_recv_packet(struct ds532_dev *dev, u8 *buffer, size_t max_len)
{
	struct file *tty_file = dev->tty_file;
	mm_segment_t old_fs;
	loff_t pos = 0;
	unsigned long timeout;
	size_t received = 0;
	int ret;
	
	if (!tty_file) {
		pr_err("[DS532] TTY not ready for read\n");
		return -ENODEV;
	}
	
	pr_debug("[DS532] Receiving packet (max %zu bytes)\n", max_len);
	
	/* 设置内核空间访问 */
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	
	/* 设置超时时间 */
	timeout = jiffies + msecs_to_jiffies(DS532_RECV_TIMEOUT_MS);
	
	/* 接收数据 */
	while (received < max_len) {
		ret = vfs_read(tty_file, buffer + received, max_len - received, &pos);
		if (ret < 0) {
			if (ret == -EAGAIN || ret == -EWOULDBLOCK) {
				/* 非阻塞模式，没有数据可读 */
				msleep(10);
				goto check_timeout;
			}
			pr_err("[DS532] TTY read error: %d\n", ret);
			set_fs(old_fs);
			return ret;
		}
		
		if (ret > 0) {
			received += ret;
			
			/* 检查是否接收到完整的包头 */
			if (received >= 2) {
				if (buffer[0] != DS532_HEADER_H || 
				    buffer[1] != DS532_HEADER_L) {
					pr_err("[DS532] Invalid packet header: 0x%02X%02X\n",
					       buffer[0], buffer[1]);
					set_fs(old_fs);
					return -EBADMSG;
				}
			}
			
			/* 检查是否接收到长度字段 */
			if (received >= 9) {
				u16 pkg_len = (buffer[7] << 8) | buffer[8];
				size_t total_len = 9 + pkg_len;
				
				/* 如果接收到完整数据包，退出 */
				if (received >= total_len) {
					pr_debug("[DS532] Complete packet received: %zu bytes\n",
						 received);
					break;
				}
			}
		}
		
check_timeout:
		/* 检查超时 */
		if (time_after(jiffies, timeout)) {
			pr_err("[DS532] Receive timeout: got %zu bytes\n", received);
			set_fs(old_fs);
			return received > 0 ? received : -ETIMEDOUT;
		}
		
		/* 短暂延时后继续接收 */
		if (ret == 0)
			msleep(10);
	}
	
	set_fs(old_fs);
	
	/* 打印接收的数据包（调试用） */
	if (received > 0 && received <= 32) {
		print_hex_dump(KERN_DEBUG, "[DS532] RX: ", DUMP_PREFIX_NONE,
			       16, 1, buffer, received, true);
	}
	
	pr_debug("[DS532] Packet received: %zu bytes\n", received);
	return received;
}

/*
 * ============================================================================
 * DS532协议封装函数实现（Stage 2）
 * ============================================================================
 */

/**
 * ds532_calculate_checksum - 计算数据包校验和
 * @data: 数据指针（从包标识开始）
 * @len: 数据长度
 * 
 * 校验和 = 包标识 + 长度 + 数据内容 的累加和
 * 
 * 返回值：16位校验和
 */
u16 ds532_calculate_checksum(const u8 *data, size_t len)
{
	u16 sum = 0;
	size_t i;
	
	for (i = 0; i < len; i++)
		sum += data[i];
	
	return sum;
}

/**
 * ds532_build_packet - 构造DS532数据包
 * @buffer: 输出缓冲区
 * @pid: 包标识（DS532_PID_CMD等）
 * @cmd: 命令码
 * @data: 数据内容（可为NULL）
 * @data_len: 数据长度
 * 
 * 数据包格式：
 *   包头(2) + 地址(4) + 包标识(1) + 长度(2) + 数据(N) + 校验和(2)
 * 
 * 返回值：数据包总长度
 */
int ds532_build_packet(u8 *buffer, u8 pid, u8 cmd, const u8 *data, u16 data_len)
{
	u16 pkg_len;
	u16 checksum;
	int offset = 0;
	
	/* 包头：0xEF01 */
	buffer[offset++] = DS532_HEADER_H;
	buffer[offset++] = DS532_HEADER_L;
	
	/* 地址：0xFFFFFFFF */
	buffer[offset++] = (DS532_DEFAULT_ADDR >> 24) & 0xFF;
	buffer[offset++] = (DS532_DEFAULT_ADDR >> 16) & 0xFF;
	buffer[offset++] = (DS532_DEFAULT_ADDR >> 8) & 0xFF;
	buffer[offset++] = DS532_DEFAULT_ADDR & 0xFF;
	
	/* 包标识 */
	buffer[offset++] = pid;
	
	/* 长度 = 包标识(1) + 命令码(1) + 数据长度 + 校验和(2) */
	pkg_len = 1 + 1 + data_len + 2;
	buffer[offset++] = (pkg_len >> 8) & 0xFF;
	buffer[offset++] = pkg_len & 0xFF;
	
	/* 命令码 */
	buffer[offset++] = cmd;
	
	/* 数据内容 */
	if (data && data_len > 0) {
		memcpy(&buffer[offset], data, data_len);
		offset += data_len;
	}
	
	/* 计算校验和（从包标识开始到数据结束） */
	checksum = ds532_calculate_checksum(&buffer[6], offset - 6);
	buffer[offset++] = (checksum >> 8) & 0xFF;
	buffer[offset++] = checksum & 0xFF;
	
	return offset;
}

/**
 * ds532_build_vfypwd_packet - 构造验证密码命令包
 * @buffer: 输出缓冲区
 * @password: 密码（4字节）
 * 
 * 命令格式：VfyPwd + 密码(4字节)
 * 
 * 返回值：数据包总长度
 */
int ds532_build_vfypwd_packet(u8 *buffer, u32 password)
{
	u8 data[4];
	
	/* 密码数据（大端序） */
	data[0] = (password >> 24) & 0xFF;
	data[1] = (password >> 16) & 0xFF;
	data[2] = (password >> 8) & 0xFF;
	data[3] = password & 0xFF;
	
	return ds532_build_packet(buffer, DS532_PID_CMD, DS532_CMD_VFYPWD, data, 4);
}

/**
 * ds532_build_genimg_packet - 构造采集图像命令包
 * @buffer: 输出缓冲区
 * 
 * 命令格式：GenImg（无参数）
 * 
 * 返回值：数据包总长度
 */
int ds532_build_genimg_packet(u8 *buffer)
{
	return ds532_build_packet(buffer, DS532_PID_CMD, DS532_CMD_GETIMAGE, NULL, 0);
}

/**
 * ds532_build_img2tz_packet - 构造生成特征命令包
 * @buffer: 输出缓冲区
 * @buffer_id: 缓冲区ID（1或2）
 * 
 * 命令格式：Img2Tz + 缓冲区ID(1字节)
 * 
 * 返回值：数据包总长度
 */
int ds532_build_img2tz_packet(u8 *buffer, u8 buffer_id)
{
	return ds532_build_packet(buffer, DS532_PID_CMD, DS532_CMD_GENCHAR, &buffer_id, 1);
}

/**
 * ds532_build_match_packet - 构造精确比对命令包
 * @buffer: 输出缓冲区
 * 
 * 命令格式：Match（无参数，比对缓冲区1和2）
 * 
 * 返回值：数据包总长度
 */
int ds532_build_match_packet(u8 *buffer)
{
	return ds532_build_packet(buffer, DS532_PID_CMD, DS532_CMD_MATCH, NULL, 0);
}

/**
 * ds532_build_search_packet - 构造搜索指纹命令包
 * @buffer: 输出缓冲区
 * @buffer_id: 缓冲区ID（1或2）
 * @start_page: 起始页码
 * @page_num: 搜索页数
 * 
 * 命令格式：Search + 缓冲区ID(1) + 起始页(2) + 页数(2)
 * 
 * 返回值：数据包总长度
 */
int ds532_build_search_packet(u8 *buffer, u8 buffer_id, u16 start_page, u16 page_num)
{
	u8 data[5];
	
	data[0] = buffer_id;
	data[1] = (start_page >> 8) & 0xFF;
	data[2] = start_page & 0xFF;
	data[3] = (page_num >> 8) & 0xFF;
	data[4] = page_num & 0xFF;
	
	return ds532_build_packet(buffer, DS532_PID_CMD, DS532_CMD_SEARCH, data, 5);
}

/**
 * ds532_build_store_packet - 构造存储特征命令包
 * @buffer: 输出缓冲区
 * @buffer_id: 缓冲区ID（1或2）
 * @page_id: 存储页码
 * 
 * 命令格式：Store + 缓冲区ID(1) + 页码(2)
 * 
 * 返回值：数据包总长度
 */
int ds532_build_store_packet(u8 *buffer, u8 buffer_id, u16 page_id)
{
	u8 data[3];
	
	data[0] = buffer_id;
	data[1] = (page_id >> 8) & 0xFF;
	data[2] = page_id & 0xFF;
	
	return ds532_build_packet(buffer, DS532_PID_CMD, DS532_CMD_STORE, data, 3);
}

/**
 * ds532_verify_packet - 验证响应包
 * @buffer: 响应包数据
 * @len: 数据长度
 * 
 * 验证包头、地址和校验和
 * 
 * 返回值：
 *   0 - 验证成功
 *   -EINVAL - 包格式错误
 *   -EBADMSG - 校验和错误
 */
int ds532_verify_packet(const u8 *buffer, size_t len)
{
	u16 pkg_len;
	u16 checksum_recv, checksum_calc;
	
	/* 最小长度检查：包头(2) + 地址(4) + 包标识(1) + 长度(2) + 确认码(1) + 校验和(2) = 12 */
	if (len < 12) {
		pr_err("[DS532] Packet too short: %zu bytes\n", len);
		return -EINVAL;
	}
	
	/* 验证包头 */
	if (buffer[0] != DS532_HEADER_H || buffer[1] != DS532_HEADER_L) {
		pr_err("[DS532] Invalid packet header: 0x%02X%02X\n", buffer[0], buffer[1]);
		return -EINVAL;
	}
	
	/* 验证地址 */
	if (buffer[2] != 0xFF || buffer[3] != 0xFF || 
	    buffer[4] != 0xFF || buffer[5] != 0xFF) {
		pr_warn("[DS532] Non-default address in response\n");
	}
	
	/* 获取包长度 */
	pkg_len = (buffer[7] << 8) | buffer[8];
	
	/* 验证总长度 */
	if (len < (9 + pkg_len)) {
		pr_err("[DS532] Packet length mismatch: expected %u, got %zu\n",
		       9 + pkg_len, len);
		return -EINVAL;
	}
	
	/* 验证校验和 */
	checksum_recv = (buffer[len - 2] << 8) | buffer[len - 1];
	checksum_calc = ds532_calculate_checksum(&buffer[6], len - 8);
	
	if (checksum_recv != checksum_calc) {
		pr_err("[DS532] Checksum error: expected 0x%04X, got 0x%04X\n",
		       checksum_calc, checksum_recv);
		return -EBADMSG;
	}
	
	return 0;
}

/**
 * ds532_parse_response - 解析响应包
 * @buffer: 响应包数据
 * @len: 数据长度
 * @ack_code: 输出：确认码
 * @data: 输出：附加数据（可为NULL）
 * @data_len: 输出：附加数据长度（可为NULL）
 * 
 * 解析响应包，提取确认码和附加数据
 * 
 * 返回值：
 *   0 - 成功
 *   <0 - 错误码
 */
int ds532_parse_response(const u8 *buffer, size_t len, u8 *ack_code, 
			 u8 *data, u16 *data_len)
{
	u16 pkg_len;
	u16 extra_len;
	int ret;
	
	/* 验证数据包 */
	ret = ds532_verify_packet(buffer, len);
	if (ret < 0)
		return ret;
	
	/* 获取包长度 */
	pkg_len = (buffer[7] << 8) | buffer[8];
	
	/* 提取确认码 */
	*ack_code = buffer[9];
	
	/* 计算附加数据长度：包长度 - 包标识(1) - 确认码(1) - 校验和(2) */
	extra_len = pkg_len - 4;
	
	/* 提取附加数据 */
	if (data && data_len && extra_len > 0) {
		*data_len = extra_len;
		memcpy(data, &buffer[10], extra_len);
	} else if (data_len) {
		*data_len = 0;
	}
	
	pr_debug("[DS532] Response parsed: ACK=0x%02X, data_len=%u\n",
		 *ack_code, extra_len);
	
	return 0;
}

/*
 * ============================================================================
 * GPIO中断处理函数实现（Stage 4）
 * ============================================================================
 */

/**
 * ds532_touch_irq_handler - 触摸中断处理函数
 * @irq: 中断号
 * @dev_id: 设备私有数据指针
 * 
 * 当指纹模块被触摸时触发，记录触摸事件
 * 
 * 返回值：IRQ_HANDLED - 中断已处理
 */
static irqreturn_t ds532_touch_irq_handler(int irq, void *dev_id)
{
	struct ds532_dev *dev = (struct ds532_dev *)dev_id;
	
	/* 记录触摸时间戳 */
	dev->last_touch_time = ktime_get();
	
	/* 增加触摸计数 */
	dev->touch_count++;
	
	pr_info("[DS532] Touch event detected! Count: %lu, Time: %lld ns\n",
		dev->touch_count, ktime_to_ns(dev->last_touch_time));
	
	return IRQ_HANDLED;
}

/**
 * ds532_gpio_init - 初始化GPIO中断
 * @dev: 设备私有数据指针
 * 
 * 从设备树获取GPIO配置，请求GPIO并注册中断处理函数
 * 
 * 返回值：
 *   0 - 成功
 *   <0 - 错误码
 */
static int ds532_gpio_init(struct ds532_dev *dev)
{
	struct device_node *np = dev->pdev->dev.of_node;
	int ret;
	
	pr_info("[DS532] Initializing GPIO interrupt\n");
	
	/* 从设备树获取GPIO引脚 */
	dev->touch_irq_gpio = of_get_named_gpio(np, "touch-irq-gpios", 0);
	if (!gpio_is_valid(dev->touch_irq_gpio)) {
		pr_err("[DS532] Invalid GPIO pin: %d\n", dev->touch_irq_gpio);
		return -EINVAL;
	}
	
	pr_info("[DS532] Touch IRQ GPIO: %d\n", dev->touch_irq_gpio);
	
	/* 请求GPIO */
	ret = devm_gpio_request(&dev->pdev->dev, dev->touch_irq_gpio, "ds532-touch-irq");
	if (ret < 0) {
		pr_err("[DS532] Failed to request GPIO %d: %d\n", dev->touch_irq_gpio, ret);
		return ret;
	}
	
	/* 配置GPIO为输入模式 */
	ret = gpio_direction_input(dev->touch_irq_gpio);
	if (ret < 0) {
		pr_err("[DS532] Failed to set GPIO direction: %d\n", ret);
		return ret;
	}
	
	/* 获取中断号 */
	dev->touch_irq = gpio_to_irq(dev->touch_irq_gpio);
	if (dev->touch_irq < 0) {
		pr_err("[DS532] Failed to get IRQ for GPIO %d: %d\n", 
		       dev->touch_irq_gpio, dev->touch_irq);
		return dev->touch_irq;
	}
	
	pr_info("[DS532] Touch IRQ number: %d\n", dev->touch_irq);
	
	/* 初始化触摸计数和时间戳 */
	dev->touch_count = 0;
	dev->last_touch_time = ktime_set(0, 0);
	
	/* 注册中断处理函数 */
	ret = request_irq(dev->touch_irq, ds532_touch_irq_handler,
			  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			  "ds532-touch", dev);
	if (ret < 0) {
		pr_err("[DS532] Failed to request IRQ %d: %d\n", dev->touch_irq, ret);
		return ret;
	}
	
	pr_info("[DS532] GPIO interrupt initialized successfully\n");
	return 0;
}

/**
 * ds532_gpio_cleanup - 清理GPIO资源
 * @dev: 设备私有数据指针
 */
static void ds532_gpio_cleanup(struct ds532_dev *dev)
{
	if (dev->touch_irq > 0) {
		pr_info("[DS532] Freeing IRQ %d\n", dev->touch_irq);
		free_irq(dev->touch_irq, dev);
		dev->touch_irq = 0;
	}
	
	/* GPIO资源由devm_gpio_request自动释放 */
	pr_info("[DS532] GPIO cleanup completed\n");
}

/*
 * ============================================================================
 * sysfs属性实现（Stage 4）
 * ============================================================================
 */

/**
 * touch_count_show - 显示触摸事件计数
 * @dev: 设备对象
 * @attr: 设备属性
 * @buf: 输出缓冲区
 * 
 * 返回值：写入缓冲区的字节数
 */
static ssize_t touch_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ds532_dev *ds532_dev = dev_get_drvdata(dev);
	return sprintf(buf, "%lu\n", ds532_dev->touch_count);
}

/**
 * last_touch_time_show - 显示最后触摸时间
 * @dev: 设备对象
 * @attr: 设备属性
 * @buf: 输出缓冲区
 * 
 * 返回值：写入缓冲区的字节数
 */
static ssize_t last_touch_time_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ds532_dev *ds532_dev = dev_get_drvdata(dev);
	s64 time_ns = ktime_to_ns(ds532_dev->last_touch_time);
	return sprintf(buf, "%lld\n", time_ns);
}

/* 定义sysfs属性 */
static DEVICE_ATTR_RO(touch_count);
static DEVICE_ATTR_RO(last_touch_time);

/* sysfs属性组 */
static struct attribute *ds532_attrs[] = {
	&dev_attr_touch_count.attr,
	&dev_attr_last_touch_time.attr,
	NULL,
};

static const struct attribute_group ds532_attr_group = {
	.attrs = ds532_attrs,
};

