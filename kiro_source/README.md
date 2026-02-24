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

### 1. 推送到开发板

```bash
make install
```

或手动推送：

```bash
adb push ds532_driver.ko /root/ds532/
adb push ds532_test /root/ds532/
```

### 2. 加载驱动

```bash
# 在开发板上执行
insmod /root/ds532/ds532_driver.ko
```

### 3. 验证设备节点

```bash
ls -l /dev/ds532_fp
```

应该看到设备节点已创建。

### 4. 运行测试程序

```bash
cd /root/ds532
./ds532_test
```

## 使用说明

### 测试程序菜单

```
DS532指纹模块测试程序
====================
1. 模块初始化 (VfyPwd)
2. 采集指纹图像 (GenImg)
3. 生成特征 (Img2Tz)
4. 指纹匹配 (Match)
5. 指纹搜索 (Search)
6. 存储指纹 (Store)
7. 删除指纹 (Delete)
8. 清空指纹库 (Empty)
9. 查看触摸事件
0. 退出

请选择:
```

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
