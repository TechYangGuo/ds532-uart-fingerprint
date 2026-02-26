

/* Platform驱动函数 */
static int ds532_probe(struct platform_device *pdev);
static int ds532_remove(struct platform_device *pdev);

/*
 * ============================================================================
 * 字符设备文件操作函数
 * ============================================================================
 */

/**
 * ds532_open - 打开设备
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
 * Stage 2: 返回模拟的响应数据用于测试
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
	
	/* Stage 2: 返回缓冲区中的数据 */
	if (dev->rx_len == 0) {
		pr_info("[DS532] No data to read\n");
		ret = 0;
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
 * Stage 2: 接收数据并使用协议封装函数验证
 */
static ssize_t ds532_write(struct file *filp, const char __user *buf,
			   size_t count, loff_t *offset)
{
	struct ds532_dev *dev = filp->private_data;
	ssize_t ret;
	int verify_ret;
	
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
	
	/* Stage 2: 验证数据包格式 */
	verify_ret = ds532_verify_packet(dev->tx_buffer, count);
	if (verify_ret == 0) {
		pr_info("[DS532] Packet verification passed\n");
	} else {
		pr_warn("[DS532] Packet verification failed: %d\n", verify_ret);
	}
	
	/* 打印发送的数据包（调试用） */
	if (count <= 32) {
		print_hex_dump(KERN_INFO, "[DS532] TX: ", DUMP_PREFIX_NONE,
			       16, 1, dev->tx_buffer, count, true);
	}
	
	ret = count;
	pr_debug("[DS532] Write %zd bytes\n", ret);
	
out:
	mutex_unlock(&dev->io_mutex);
	return ret;
}

/**
 * ds532_ioctl - 设备控制操作
 * Stage 2: 使用协议封装函数构造命令包
 */
static long ds532_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct ds532_dev *dev = filp->private_data;
	int ret = 0;
	int len;
	
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
		/* 使用协议封装函数构造VfyPwd命令包 */
		len = ds532_build_vfypwd_packet(dev->rx_buffer, 0x00000000);
		dev->rx_len = len;
		pr_info("[DS532] Built VfyPwd packet: %d bytes\n", len);
		print_hex_dump(KERN_INFO, "[DS532] VfyPwd: ", DUMP_PREFIX_NONE,
			       16, 1, dev->rx_buffer, len, true);
		ret = 0;
		break;
		
	case DS532_IOC_GETIMAGE:
		pr_info("[DS532] IOCTL: GetImage\n");
		/* 使用协议封装函数构造GenImg命令包 */
		len = ds532_build_genimg_packet(dev->rx_buffer);
		dev->rx_len = len;
		pr_info("[DS532] Built GenImg packet: %d bytes\n", len);
		print_hex_dump(KERN_INFO, "[DS532] GenImg: ", DUMP_PREFIX_NONE,
			       16, 1, dev->rx_buffer, len, true);
		ret = 0;
		break;
		
	case DS532_IOC_GENCHAR:
		pr_info("[DS532] IOCTL: GenChar\n");
		/* 使用协议封装函数构造Img2Tz命令包 */
		len = ds532_build_img2tz_packet(dev->rx_buffer, 1);
		dev->rx_len = len;
		pr_info("[DS532] Built Img2Tz packet: %d bytes\n", len);
		print_hex_dump(KERN_INFO, "[DS532] Img2Tz: ", DUMP_PREFIX_NONE,
			       16, 1, dev->rx_buffer, len, true);
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
 */
static int ds532_probe(struct platform_device *pdev)
{
	struct ds532_dev *dev;
	int ret;
	
	pr_info("[DS532] Probing DS532 driver (Stage 2)\n");
	
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
	
	/* 初始化缓冲区 */
	dev->rx_len = 0;
	
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
	
	pr_info("[DS532] Driver initialized successfully (Stage 2)\n");
	pr_info("[DS532] Device node: /dev/%s\n", DEVICE_NAME);
	pr_info("[DS532] Stage 2: Protocol encapsulation functions available\n");
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
 */
static int ds532_remove(struct platform_device *pdev)
{
	struct ds532_dev *dev = platform_get_drvdata(pdev);
	
	pr_info("[DS532] Removing DS532 driver (Stage 2)\n");
	
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
MODULE_DESCRIPTION("DS532 Fingerprint UART Driver - Stage 2");
MODULE_VERSION("0.2-stage2");

/*
 * ============================================================================
 * DS532协议封装函数实现（Stage 2）
 * ============================================================================
 */

/**
 * ds532_calculate_checksum - 计算数据包校验和
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
 */
int ds532_build_genimg_packet(u8 *buffer)
{
	return ds532_build_packet(buffer, DS532_PID_CMD, DS532_CMD_GETIMAGE, NULL, 0);
}

/**
 * ds532_build_img2tz_packet - 构造生成特征命令包
 */
int ds532_build_img2tz_packet(u8 *buffer, u8 buffer_id)
{
	return ds532_build_packet(buffer, DS532_PID_CMD, DS532_CMD_GENCHAR, &buffer_id, 1);
}

/**
 * ds532_build_match_packet - 构造精确比对命令包
 */
int ds532_build_match_packet(u8 *buffer)
{
	return ds532_build_packet(buffer, DS532_PID_CMD, DS532_CMD_MATCH, NULL, 0);
}

/**
 * ds532_build_search_packet - 构造搜索指纹命令包
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
 */
int ds532_verify_packet(const u8 *buffer, size_t len)
{
	u16 pkg_len;
	u16 checksum_recv, checksum_calc;
	
	/* 最小长度检查 */
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
	
	/* 计算附加数据长度 */
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
