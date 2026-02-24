# Fingerprint UART Driver

## 项目概述

- 目标: 为 IMX6ULL 开发板编写 UART 指纹模块驱动
- 平台: NXP IMX6ULL, Linux 4.9.88
- 接口: UART6 (/dev/ttymxc5)

## 硬件连接

- TX -> UART6_RX
- RX -> UART6_TX
- GND -> GND
- VCC -> 3.3V
- 触摸中断 -> GPIO4_IO21 (J1.0)

## 文件说明

- fingerprint_uart.c - Linux 内核驱动
- fingerprint_test.c - 用户空间测试程序
- fingerprint.dts - 设备树配置
- Makefile - 编译配置
- README.md - 使用文档

## 编译

驱动:
  cd /home/book/Workspace/project1/smart_face/fingerprint
  make

测试程序:
  arm-buildroot-linux-gnueabihf-gcc -o fingerprint_test fingerprint_test.c

## 部署

adb push fingerprint_uart.ko /root/
adb push fingerprint_test /root/

## 使用

加载驱动:
  insmod fingerprint_uart.ko

运行测试:
  ./fingerprint_test

## 调试

  dmesg | grep fingerprint

## 注意

1. 波特率需与指纹模块匹配(默认57600)
2. 协议需根据实际模块手册调整
3. GPIO触摸中断引脚需根据实际接线修改
4. 设备采用独占访问模式，同一时刻只允许一个进程打开设备
