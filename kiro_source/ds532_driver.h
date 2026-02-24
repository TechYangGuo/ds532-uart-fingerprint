/*
 * DS532指纹模块UART驱动 - 头文件
 * 
 * 版权所有 (C) 2026
 * 作者: Kiro开发团队
 * 
 * 本驱动实现DS532方形指纹模块的Linux UART驱动
 * 平台: NXP IMX6ULL
 * 内核: Linux 4.9.88
 * 接口: UART6 (/dev/ttymxc5)
 */

#ifndef __DS532_DRIVER_H__
#define __DS532_DRIVER_H__

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/atomic.h>

/*
 * ============================================================================
 * DS532协议常量定义
 * ============================================================================
 */

/* 包头定义 */
#define DS532_HEADER_H			0xEF
#define DS532_HEADER_L			0x01

/* 包标识定义 */
#define DS532_PID_CMD			0x01	/* 命令包 */
#define DS532_PID_DATA			0x02	/* 数据包 */
#define DS532_PID_ACK			0x07	/* 应答包 */
#define DS532_PID_END			0x08	/* 结束包 */

/* 命令码定义 */
#define DS532_CMD_GETIMAGE		0x01	/* 采集指纹图像 */
#define DS532_CMD_GENCHAR		0x02	/* 生成特征 */
#define DS532_CMD_MATCH			0x03	/* 精确比对 */
#define DS532_CMD_SEARCH		0x04	/* 搜索指纹 */
#define DS532_CMD_REGMODEL		0x05	/* 合并特征 */
#define DS532_CMD_STORE			0x06	/* 存储模板 */
#define DS532_CMD_LOAD			0x07	/* 读取模板 */
#define DS532_CMD_DELETE		0x0C	/* 删除模板 */
#define DS532_CMD_EMPTY			0x0D	/* 清空指纹库 */
#define DS532_CMD_VFYPWD		0x13	/* 验证密码 */
#define DS532_CMD_SETPWD		0x12	/* 设置密码 */
#define DS532_CMD_READPARA		0x0F	/* 读取参数 */

/* 确认码定义 */
#define DS532_ACK_SUCCESS		0x00	/* 成功 */
#define DS532_ACK_ERR_RECV		0x01	/* 接收包错误 */
#define DS532_ACK_ERR_NOFINGER		0x02	/* 无手指 */
#define DS532_ACK_ERR_ENROLL		0x03	/* 录入失败 */
#define DS532_ACK_ERR_DISORDER		0x06	/* 图像混乱 */
#define DS532_ACK_ERR_FEWFEATURE	0x07	/* 特征点太少 */
#define DS532_ACK_ERR_NOMATCH		0x08	/* 不匹配 */
#define DS532_ACK_ERR_NOTFOUND		0x09	/* 未搜索到 */
#define DS532_ACK_ERR_MERGE		0x0A	/* 合并失败 */
#define DS532_ACK_ERR_BADLOC		0x0B	/* 地址超出范围 */
#define DS532_ACK_ERR_FLASH		0x18	/* Flash错误 */

/* 缓冲区大小 */
#define DS532_BUFFER_SIZE		256

/* 默认模块地址 */
#define DS532_DEFAULT_ADDR		0xFFFFFFFF

/* 超时定义（毫秒） */
#define DS532_SEND_TIMEOUT_MS		500
#define DS532_RECV_TIMEOUT_MS		2000

/*
 * ============================================================================
 * IOCTL命令定义
 * ============================================================================
 */

#define DS532_IOC_MAGIC			'F'

#define DS532_IOC_INIT			_IO(DS532_IOC_MAGIC, 0)
#define DS532_IOC_GETIMAGE		_IO(DS532_IOC_MAGIC, 1)
#define DS532_IOC_GENCHAR		_IOW(DS532_IOC_MAGIC, 2, int)
#define DS532_IOC_MATCH			_IOR(DS532_IOC_MAGIC, 3, int)
#define DS532_IOC_SEARCH		_IOWR(DS532_IOC_MAGIC, 4, struct ds532_search_param)
#define DS532_IOC_STORE			_IOW(DS532_IOC_MAGIC, 5, struct ds532_store_param)
#define DS532_IOC_DELETE		_IOW(DS532_IOC_MAGIC, 6, int)
#define DS532_IOC_EMPTY			_IO(DS532_IOC_MAGIC, 7)

/*
 * ============================================================================
 * 数据结构定义
 * ============================================================================
 */

/**
 * struct ds532_search_param - 搜索指纹参数
 * @buffer_id: 缓冲区ID (1或2)
 * @start_page: 起始页
 * @page_num: 页数
 * @page_id: 返回：匹配的页码
 * @score: 返回：匹配得分
 */
struct ds532_search_param {
	u8 buffer_id;
	u16 start_page;
	u16 page_num;
	u16 page_id;
	u16 score;
};

/**
 * struct ds532_store_param - 存储指纹参数
 * @buffer_id: 缓冲区ID (1或2)
 * @page_id: 存储位置
 */
struct ds532_store_param {
	u8 buffer_id;
	u16 page_id;
};

/**
 * struct ds532_dev - DS532设备私有数据
 * @cdev: 字符设备结构体
 * @devno: 设备号
 * @class: 设备类
 * @device: 设备对象
 * @tty_file: TTY设备文件指针
 * @tty: TTY结构体指针
 * @baudrate: 波特率配置
 * @touch_irq_gpio: GPIO引脚号
 * @touch_irq: 中断号
 * @touch_count: 触摸事件计数
 * @last_touch_time: 最后一次触摸时间戳
 * @io_mutex: UART操作互斥锁
 * @device_opened: 设备打开标志（原子变量）
 * @tx_buffer: 发送缓冲区
 * @rx_buffer: 接收缓冲区
 * @rx_len: 接收数据长度
 * @pdev: 平台设备指针
 */
struct ds532_dev {
	/* 字符设备相关 */
	struct cdev cdev;
	dev_t devno;
	struct class *class;
	struct device *device;
	
	/* TTY/UART相关（后续阶段实现） */
	struct file *tty_file;
	struct tty_struct *tty;
	u32 baudrate;
	
	/* GPIO中断相关（后续阶段实现） */
	int touch_irq_gpio;
	int touch_irq;
	unsigned long touch_count;
	ktime_t last_touch_time;
	
	/* 并发控制 */
	struct mutex io_mutex;
	atomic_t device_opened;
	
	/* 数据缓冲区 */
	u8 tx_buffer[DS532_BUFFER_SIZE];
	u8 rx_buffer[DS532_BUFFER_SIZE];
	size_t rx_len;
	
	/* 平台设备 */
	struct platform_device *pdev;
};

#endif /* __DS532_DRIVER_H__ */
