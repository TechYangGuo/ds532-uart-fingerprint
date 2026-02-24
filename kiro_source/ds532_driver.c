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
 * - Platform驱动注册
 * - 字符设备创建
 * - 基本的open/close/read/write/ioctl接口
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

#include "ds532_driver.h"

/* 设备名称和类名 */
#define DEVICE_NAME	"ds532_fp"
#define CLASS_NAME	"ds532"

/* 全局设备指针 */
static struct ds532_dev *g_ds532_dev;

/*
 * ============================================================================
 * 字符设备文件操作函数
 * ============================================================================
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
	
	/* 检查是否有数据 */
	if (dev->rx_len == 0) {
		pr_debug("[DS532] No data available\n");
		goto out;
	}
	
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
	
	/* TODO: 第2/3阶段实现实际的UART发送 */
	/* 目前只是模拟成功 */
	ret = count;
	pr_info("[DS532] Write %zd bytes (simulated)\n", ret);
	
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
	
	pr_info("[DS532] Driver initialized successfully\n");
	pr_info("[DS532] Device node: /dev/%s\n", DEVICE_NAME);
	return 0;

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
MODULE_VERSION("0.1-stage1");
