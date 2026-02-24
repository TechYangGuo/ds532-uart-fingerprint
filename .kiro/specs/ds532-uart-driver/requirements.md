# 需求文档 - DS532指纹模块UART驱动

## 引言

本文档定义了DS532方形指纹模块Linux UART驱动的功能需求。该驱动运行在NXP IMX6ULL开发板上，Linux内核版本4.9.88，通过UART6接口（/dev/ttymxc5）与DS532指纹模块通信。

本项目的主要目标是：
1. 开发一个完整的、生产级别的UART指纹模块Linux驱动
2. 学习和掌握Linux UART设备驱动开发的最佳实践
3. 建立可复现、可复用的嵌入式驱动开发规范

## 术语表

- **DS532_Driver**: DS532指纹模块的Linux内核驱动程序
- **TTY_Subsystem**: Linux内核的TTY（终端）子系统，用于串口通信
- **Platform_Driver**: Linux平台设备驱动框架
- **Device_Tree**: 设备树，用于描述硬件配置的数据结构
- **Character_Device**: 字符设备，提供用户空间访问接口
- **UART6**: IMX6ULL的第6个UART硬件接口（对应/dev/ttymxc5）
- **GPIO_Touch_IRQ**: GPIO触摸中断引脚（GPIO4_IO21），当指纹模块被触摸时产生电平变化，通知SOC有触摸事件发生
- **DS532_Protocol**: DS532指纹模块的通信协议
- **User_Application**: 用户空间测试应用程序
- **Fingerprint_Module**: DS532方形指纹识别硬件模块
- **Command_Packet**: 发送给指纹模块的命令数据包
- **Response_Packet**: 指纹模块返回的响应数据包
- **Confirmation_Code**: 指纹模块返回的确认码，表示命令执行状态
- **Mutex_Lock**: 互斥锁，用于实现设备的独占访问，确保同一时刻只有一个进程可以访问UART设备

## 需求

### 需求 1: 驱动初始化与设备注册

**用户故事**: 作为系统开发者，我希望驱动能够正确初始化并注册到内核，以便系统能够识别和管理指纹模块设备。

#### 验收标准

1. WHEN 驱动模块加载时，THE DS532_Driver SHALL 从Device_Tree读取硬件配置参数
2. WHEN 驱动模块加载时，THE DS532_Driver SHALL 分配字符设备号并创建Character_Device节点
3. WHEN 驱动模块加载时，THE DS532_Driver SHALL 在/dev目录下创建名为"ds532_fp"的设备节点
4. WHEN 驱动模块加载时，THE DS532_Driver SHALL 初始化GPIO_Touch_IRQ引脚为输入模式，并配置为中断触发模式
5. WHEN 驱动模块加载时，THE DS532_Driver SHALL 打开UART6接口并配置为57600波特率、8数据位、无校验、1停止位
6. IF 任何初始化步骤失败，THEN THE DS532_Driver SHALL 释放已分配的资源并返回错误码
7. WHEN 驱动初始化成功时，THE DS532_Driver SHALL 在内核日志中输出"DS532 driver initialized successfully"消息

### 需求 2: UART通信管理

**用户故事**: 作为驱动开发者，我希望驱动能够通过UART与指纹模块进行可靠的数据通信，以便实现指纹识别功能。

#### 验收标准

1. THE DS532_Driver SHALL 通过TTY_Subsystem与UART6进行数据收发
2. WHEN 发送数据时，THE DS532_Driver SHALL 将数据写入TTY设备的发送缓冲区
3. WHEN 接收数据时，THE DS532_Driver SHALL 从TTY设备的接收缓冲区读取数据
4. WHEN 发送Command_Packet时，THE DS532_Driver SHALL 在500毫秒内完成发送操作
5. WHEN 等待Response_Packet时，THE DS532_Driver SHALL 设置2000毫秒的接收超时
6. IF 接收超时发生，THEN THE DS532_Driver SHALL 返回超时错误码
7. THE DS532_Driver SHALL 维护发送和接收缓冲区，每个缓冲区大小至少为256字节
8. WHEN 多个进程同时访问设备时，THE DS532_Driver SHALL 使用Mutex_Lock保护UART访问，实现设备的独占访问模式（同一时刻只允许一个进程打开设备）

### 需求 3: DS532协议封装

**用户故事**: 作为驱动开发者，我希望驱动能够封装DS532协议的基本命令，以便用户空间程序能够方便地控制指纹模块。

#### 验收标准

1. THE DS532_Driver SHALL 实现握手验证命令（VfyPwd）的封装函数
2. THE DS532_Driver SHALL 实现采集指纹图像命令（GenImg）的封装函数
3. THE DS532_Driver SHALL 实现生成特征命令（Img2Tz）的封装函数
4. THE DS532_Driver SHALL 实现特征匹配命令（Match）的封装函数
5. THE DS532_Driver SHALL 实现指纹搜索命令（Search）的封装函数
6. THE DS532_Driver SHALL 实现存储特征命令（Store）的封装函数
7. WHEN 构造Command_Packet时，THE DS532_Driver SHALL 按照DS532_Protocol格式填充包头、地址、包标识、长度、指令码和校验和
8. WHEN 解析Response_Packet时，THE DS532_Driver SHALL 验证包头、地址和校验和的正确性
9. IF 校验和验证失败，THEN THE DS532_Driver SHALL 返回校验错误码
10. WHEN 解析Response_Packet时，THE DS532_Driver SHALL 提取Confirmation_Code并返回给调用者

### 需求 4: 字符设备接口

**用户故事**: 作为应用程序开发者，我希望通过标准的文件操作接口访问指纹模块，以便在用户空间实现指纹识别应用。

#### 验收标准

1. THE DS532_Driver SHALL 实现open、release、read、write和unlocked_ioctl文件操作
2. WHEN User_Application打开设备节点时，THE DS532_Driver SHALL 检查设备是否已被其他进程占用
3. IF 设备已被占用，THEN THE DS532_Driver SHALL 返回-EBUSY错误码
4. WHEN User_Application调用write时，THE DS532_Driver SHALL 将数据作为Command_Packet发送到Fingerprint_Module
5. WHEN User_Application调用read时，THE DS532_Driver SHALL 返回最近一次接收到的Response_Packet
6. WHEN User_Application调用ioctl时，THE DS532_Driver SHALL 根据命令码执行相应的DS532协议命令
7. THE DS532_Driver SHALL 定义IOCTL命令码：FP_IOC_INIT（初始化）、FP_IOC_GETIMAGE（采集图像）、FP_IOC_GENCHAR（生成特征）、FP_IOC_MATCH（匹配）、FP_IOC_SEARCH（搜索）
8. WHEN 执行IOCTL命令时，THE DS532_Driver SHALL 在命令完成后返回Confirmation_Code

### 需求 5: GPIO中断处理

**用户故事**: 作为驱动开发者，我希望驱动能够响应指纹模块的触摸中断信号，以便及时处理用户的指纹采集请求。

#### 验收标准

1. THE DS532_Driver SHALL 从Device_Tree读取GPIO_Touch_IRQ引脚配置
2. WHEN 驱动初始化时，THE DS532_Driver SHALL 请求GPIO_Touch_IRQ引脚并配置为输入模式
3. WHEN 驱动初始化时，THE DS532_Driver SHALL 注册GPIO_Touch_IRQ引脚的中断处理函数
4. THE DS532_Driver SHALL 配置GPIO_Touch_IRQ为双边沿触发（上升沿和下降沿均触发中断）
5. WHEN Fingerprint_Module被触摸时，THE Fingerprint_Module SHALL 在GPIO_Touch_IRQ引脚产生电平变化
6. WHEN GPIO_Touch_IRQ引脚电平变化时，THE DS532_Driver SHALL 触发中断处理函数
7. WHEN 中断处理函数被调用时，THE DS532_Driver SHALL 在内核日志中记录触摸事件
8. THE DS532_Driver SHALL 提供sysfs接口，允许用户空间程序读取最近一次触摸事件的时间戳
9. WHEN 驱动卸载时，THE DS532_Driver SHALL 释放GPIO_Touch_IRQ引脚的中断资源和GPIO控制权

### 需求 6: 错误处理与日志

**用户故事**: 作为系统维护者，我希望驱动能够提供详细的错误信息和日志，以便快速定位和解决问题。

#### 验收标准

1. WHEN 发生错误时，THE DS532_Driver SHALL 返回标准的Linux错误码（-ENOMEM、-EBUSY、-ETIMEDOUT、-EIO等）
2. WHEN 发生错误时，THE DS532_Driver SHALL 在内核日志中输出错误描述信息
3. THE DS532_Driver SHALL 使用pr_err输出错误级别日志
4. THE DS532_Driver SHALL 使用pr_info输出信息级别日志
5. THE DS532_Driver SHALL 使用pr_debug输出调试级别日志
6. WHEN 记录日志时，THE DS532_Driver SHALL 在日志消息前添加"[DS532]"前缀
7. WHEN UART通信失败时，THE DS532_Driver SHALL 记录失败的数据包内容（十六进制格式）
8. WHEN 接收到无效Response_Packet时，THE DS532_Driver SHALL 记录包的原始数据

### 需求 7: 资源管理

**用户故事**: 作为系统开发者，我希望驱动能够正确管理系统资源，以便避免内存泄漏和资源冲突。

#### 验收标准

1. THE DS532_Driver SHALL 使用devm_kzalloc分配设备私有数据结构
2. THE DS532_Driver SHALL 使用devm_gpio_request请求GPIO资源
3. WHEN 驱动卸载时，THE DS532_Driver SHALL 关闭TTY设备
4. WHEN 驱动卸载时，THE DS532_Driver SHALL 注销Character_Device
5. WHEN 驱动卸载时，THE DS532_Driver SHALL 释放字符设备号
6. WHEN 驱动卸载时，THE DS532_Driver SHALL 销毁设备类和设备节点
7. IF probe函数中任何步骤失败，THEN THE DS532_Driver SHALL 按相反顺序释放已分配的资源
8. THE DS532_Driver SHALL 使用mutex保护共享数据结构的访问

### 需求 8: 设备树配置

**用户故事**: 作为硬件集成工程师，我希望通过设备树配置硬件参数，以便在不修改代码的情况下适配不同的硬件连接。

#### 验收标准

1. THE Device_Tree SHALL 包含compatible属性，值为"ds532-uart"
2. THE Device_Tree SHALL 包含uart属性，指向UART6节点的phandle
3. THE Device_Tree SHALL 包含touch-irq-gpios属性，指定GPIO_Touch_IRQ引脚
4. THE Device_Tree SHALL 包含baudrate属性，指定UART波特率（默认57600）
5. WHERE 需要修改波特率时，THE Device_Tree SHALL 允许通过修改baudrate属性来配置
6. THE DS532_Driver SHALL 使用of_property_read_u32读取baudrate属性
7. THE DS532_Driver SHALL 使用of_get_named_gpio读取touch-irq-gpios属性

### 需求 9: 用户空间测试程序

**用户故事**: 作为驱动测试人员，我希望有一个测试程序来验证驱动功能，以便确保驱动正确实现了所有功能。

#### 验收标准

1. THE User_Application SHALL 提供交互式菜单界面
2. THE User_Application SHALL 支持以下测试功能：模块初始化、采集指纹图像、生成特征、指纹匹配、指纹搜索、存储指纹
3. WHEN 执行测试功能时，THE User_Application SHALL 通过ioctl调用驱动接口
4. WHEN 接收到驱动响应时，THE User_Application SHALL 显示Confirmation_Code和对应的状态描述
5. THE User_Application SHALL 将发送和接收的数据包以十六进制格式打印到终端
6. WHEN 发生错误时，THE User_Application SHALL 显示错误原因和建议的解决方法
7. THE User_Application SHALL 提供退出选项以正常关闭设备

### 需求 10: 代码规范与文档

**用户故事**: 作为项目维护者，我希望代码遵循统一的编码规范并包含完整的文档，以便团队成员能够理解和维护代码。

#### 验收标准

1. THE DS532_Driver SHALL 遵循Linux内核编码规范（Documentation/process/coding-style.rst）
2. THE DS532_Driver SHALL 在每个函数前添加中文注释，说明函数功能、参数和返回值
3. THE DS532_Driver SHALL 在关键代码段添加中文行内注释
4. THE DS532_Driver SHALL 在文件头部包含版权声明、作者信息和模块描述
5. THE DS532_Driver SHALL 使用tab缩进（8个空格宽度）
6. THE DS532_Driver SHALL 限制每行代码不超过80个字符
7. THE User_Application SHALL 包含中文注释说明每个测试功能的用途
8. THE 项目 SHALL 包含README.md文档，说明编译、部署和使用方法
9. THE 项目 SHALL 包含故障排查指南，列出常见问题和解决方案

### 需求 11: 并发安全

**用户故事**: 作为系统开发者，我希望驱动能够安全地处理并发访问，以便多个进程或线程不会导致数据竞争或系统崩溃。

#### 验收标准

1. THE DS532_Driver SHALL 使用Mutex_Lock保护设备打开状态，实现独占访问
2. THE DS532_Driver SHALL 使用Mutex_Lock保护UART读写操作
3. WHEN 一个进程正在执行UART操作时，THE DS532_Driver SHALL 阻塞其他进程的UART访问请求
4. THE DS532_Driver SHALL 在所有可能睡眠的代码路径中使用mutex而非spinlock
5. WHEN 持有mutex时，THE DS532_Driver SHALL 在函数返回前释放mutex
6. IF 获取mutex失败，THEN THE DS532_Driver SHALL 返回-ERESTARTSYS错误码
7. THE DS532_Driver SHALL 使用原子变量或标志位记录设备打开状态，确保同一时刻只有一个进程可以打开设备

### 需求 12: 性能要求

**用户故事**: 作为应用开发者，我希望驱动能够满足指纹识别的实时性要求，以便提供良好的用户体验。

#### 验收标准

1. WHEN 发送Command_Packet时，THE DS532_Driver SHALL 在500毫秒内完成发送
2. WHEN 执行采集指纹图像命令时，THE DS532_Driver SHALL 在3000毫秒内返回结果
3. WHEN 执行指纹匹配命令时，THE DS532_Driver SHALL 在1000毫秒内返回结果
4. WHEN 执行指纹搜索命令时，THE DS532_Driver SHALL 在5000毫秒内返回结果
5. THE DS532_Driver SHALL 支持至少每秒10次的命令执行频率
