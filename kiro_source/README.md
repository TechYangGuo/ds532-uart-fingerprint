# DS532指纹模块UART驱动

## 项目概述

本项目是DS532方形指纹模块的Linux UART驱动实现，基于完整的规范驱动开发流程。

- **目标平台**: NXP IMX6ULL
- **Linux内核**: 4.9.88
- **UART接口**: UART6 (/dev/ttymxc5)
- **GPIO中断**: GPIO4_IO21 (触摸中断输入)

## 目录说明

本目录 (`kiro_source/`) 包含DS532驱动的所有源代码和配置文件，与项目顶层目录的现有文件完全隔离。

### 文件清单

```
kiro_source/
├── ds532_driver.c          # 驱动源文件
├── ds532_driver.h          # 驱动头文件
├── ds532_test.c            # 用户空间测试程序
├── ds532_fingerprint.dts   # 设备树配置
├── Makefile                # 编译配置
└── README.md               # 本文件
```

## 规范文档

完整的规范文档位于 `../.kiro/specs/ds532-uart-driver/` 目录：

- `requirements.md` - 需求文档（12个核心需求）
- `design.md` - 设计文档（完整技术设计）
- `tasks.md` - 任务列表（14个主任务）
- `FILE_CONSTRAINTS.md` - 文件约束说明

## 编译

### 前提条件

1. 配置好Linux内核4.9.88的交叉编译环境
2. 设置好工具链路径

### 编译命令

```bash
cd kiro_source
make
```

编译成功后会生成：
- `ds532_driver.ko` - 内核驱动模块
- `ds532_test` - 用户空间测试程序

### 清理

```bash
make clean
```

## 部署

### 快速部署（推荐）

```bash
# 编译并部署到目标机
make all-with-tests && make install
```

**目标路径**: `/root/smart_lock/driver/ds532/`

### 详细部署步骤

#### 1. 编译驱动和测试程序

```bash
cd kiro_source
make all-with-tests
```

#### 2. 部署到目标机

```bash
# 完整部署（驱动+测试+文档）
make install

# 或快速部署（仅驱动+测试）
make install-quick
```

#### 3. 在目标机上加载驱动

```bash
# 连接到目标机
adb shell

# 进入驱动目录
cd /root/smart_lock/driver/ds532

# 加载驱动
insmod ds532_driver.ko

# 验证设备节点
ls -l /dev/ds532_fp

# 查看内核日志
dmesg | grep DS532 | tail -20
```

#### 4. 运行测试程序

```bash
# Stage 1测试（基础功能）
./stage1_ioctl_test1_app

# Stage 2测试（协议封装，无需硬件）
./stage2_protocol_test

# Stage 3测试（UART通信，需要硬件）
./stage3_uart_test

# Stage 4测试（GPIO中断，需要硬件）
./stage4_gpio_test
```

#### 5. 卸载驱动

```bash
rmmod ds532_driver
```

### 部署文档

详细的部署说明请参考：
- `DEPLOYMENT_GUIDE.md` - 完整部署指南
- `QUICK_DEPLOY.md` - 快速部署参考

## 使用说明

### 阶段测试程序

本项目采用分阶段开发和测试方法：

#### Stage 1: 基础驱动框架测试
```bash
./stage1_ioctl_test1_app
```
测试内容：
- 驱动加载和卸载
- 设备节点创建
- 设备独占访问
- 基本文件操作

#### Stage 2: 协议封装测试（无需硬件）
```bash
./stage2_protocol_test
```
测试内容：
- 校验和计算
- 数据包构造
- 命令封装函数
- 响应包解析

#### Stage 3: UART通信测试（需要硬件）
```bash
./stage3_uart_test
```
测试内容：
- UART设备打开
- 数据包发送
- 数据包接收
- 超时处理

#### Stage 4: GPIO中断测试（需要硬件）
```bash
./stage4_gpio_test
```
测试内容：
- GPIO中断注册
- 触摸事件检测
- sysfs接口读取

### 测试指南文档

每个阶段都有详细的测试指南：
- `STAGE1_TEST_GUIDE.md` - Stage 1测试指南
- `STAGE2_TEST_GUIDE.md` - Stage 2测试指南
- `STAGE3_TEST_GUIDE.md` - Stage 3测试指南
- `STAGE4_TEST_GUIDE.md` - Stage 4测试指南

### GPIO中断测试

查看触摸事件计数：

```bash
cat /sys/class/ds532/ds532_fp/touch_count
```

查看最后一次触摸时间：

```bash
cat /sys/class/ds532/ds532_fp/last_touch_time
```

## 调试

### 查看内核日志

```bash
dmesg | grep DS532
```

### 查看中断信息

```bash
cat /proc/interrupts | grep ds532
```

### 查看设备信息

```bash
ls -l /sys/class/ds532/
```

## 硬件连接

### UART连接

- DS532 TX -> IMX6ULL UART6_RX
- DS532 RX -> IMX6ULL UART6_TX
- DS532 GND -> IMX6ULL GND
- DS532 VCC -> IMX6ULL 3.3V

### GPIO连接

- DS532 Touch_IRQ -> IMX6ULL GPIO4_IO21 (J1.0)

## 设备树配置

设备树节点定义在 `ds532_fingerprint.dts` 中：

```dts
ds532_fingerprint: ds532-uart {
    compatible = "ds532-uart";
    uart = <&uart6>;
    touch-irq-gpios = <&gpio4 21 GPIO_ACTIVE_HIGH>;
    baudrate = <57600>;
    status = "okay";
};
```

## 故障排查

### 问题1: 设备节点未创建

**症状**: `/dev/ds532_fp` 不存在

**解决方案**:
1. 检查驱动是否加载成功：`lsmod | grep ds532`
2. 查看内核日志：`dmesg | grep DS532`
3. 检查设备树配置是否正确

### 问题2: UART通信失败

**症状**: 发送命令无响应

**解决方案**:
1. 检查UART连接是否正确
2. 检查波特率配置（应为57600）
3. 使用示波器或逻辑分析仪检查信号
4. 查看内核日志中的错误信息

### 问题3: GPIO中断不响应

**症状**: 触摸指纹模块，计数器不增加

**解决方案**:
1. 检查GPIO连接是否正确
2. 查看中断是否注册：`cat /proc/interrupts | grep ds532`
3. 检查设备树中的GPIO配置
4. 查看内核日志中的中断信息

### 问题4: 设备被占用

**症状**: 打开设备时返回 -EBUSY

**解决方案**:
1. 检查是否有其他进程正在使用设备：`lsof | grep ds532`
2. 关闭其他使用设备的进程
3. 设备采用独占访问模式，同一时刻只允许一个进程打开

## 性能指标

- 命令发送时间: < 500ms
- 采集指纹图像: < 3000ms
- 指纹匹配: < 1000ms
- 指纹搜索: < 5000ms
- 命令执行频率: ≥ 10次/秒

## 开发规范

本项目严格遵循：
- Linux内核编码规范
- 使用tab缩进（8个空格宽度）
- 每行代码不超过80个字符
- 所有函数都有中文注释
- 所有错误都有详细的日志记录

## 许可证

GPL v2

## 作者

开发团队

## 版本历史

- v0.1 - 初始版本
  - 实现基本的Platform驱动框架
  - 实现DS532协议封装
  - 实现TTY/UART通信
  - 实现GPIO中断处理
  - 实现字符设备接口
  - 实现用户空间测试程序

## 参考资料

- DS532用户手册: `../source/方形指纹DS532用户使用手册/`
- Linux内核文档: `Documentation/`
- 规范文档: `../.kiro/specs/ds532-uart-driver/`
