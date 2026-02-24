# DS532指纹模块UART驱动 Makefile
# 平台: IMX6ULL / Linux 4.9.88
# 第1阶段：最小可运行驱动框架

# 内核模块对象
obj-m += ds532_driver.o

# 内核源码路径
KDIR := /home/book/Workspace/wds/sdk/imx6ull/100ask_imx6ull-sdk/Linux-4.9.88

# 交叉编译工具链
CROSS_COMPILE := /home/book/Workspace/wds/sdk/imx6ull/100ask_imx6ull-sdk/Buildroot_2020.02.x/output/host/bin/arm-buildroot-linux-gnueabihf-
CC := /home/book/Workspace/wds/sdk/imx6ull/100ask_imx6ull-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/arm-buildroot-linux-gnueabihf-gcc

# 架构参数
ARCH_ARGS := CROSS_COMPILE=$(CROSS_COMPILE) ARCH=arm

# 当前目录
PWD := $(shell pwd)

# 编译选项
EXTRA_CFLAGS += -DDEBUG

# 默认目标：编译内核模块
all: modules

# 编译内核模块
modules:
	@echo "========================================="
	@echo "编译DS532驱动模块..."
	@echo "========================================="
	make $(ARCH_ARGS) -C $(KDIR) M=$(PWD) modules
	@echo ""
	@echo "编译完成！生成文件："
	@ls -lh ds532_driver.ko 2>/dev/null || echo "  ds532_driver.ko"
	@echo ""

# 编译测试程序（第5阶段实现）
test:
	@echo "测试程序将在第5阶段实现"

# 部署到开发板
install: modules
	@echo "========================================="
	@echo "部署DS532驱动到开发板..."
	@echo "========================================="
	adb push ds532_driver.ko /root/smart_lock/driver/
	@echo ""
	@echo "部署完成！"
	@echo ""
	@echo "在开发板上执行以下命令加载驱动："
	@echo "  insmod /root/smart_lock/driver/ds532_driver.ko"
	@echo ""
	@echo "验证设备节点："
	@echo "  ls -l /dev/ds532_fp"
	@echo ""
	@echo "查看内核日志："
	@echo "  dmesg | grep DS532"
	@echo ""

# 清理编译文件
clean:
	@echo "清理编译文件..."
	make $(ARCH_ARGS) -C $(KDIR) M=$(PWD) clean
	rm -f ds532_test
	@echo "清理完成！"

# 帮助信息
help:
	@echo "DS532驱动 Makefile 使用说明"
	@echo "========================================="
	@echo "make          - 编译驱动模块"
	@echo "make modules  - 编译驱动模块"
	@echo "make install  - 部署到开发板"
	@echo "make clean    - 清理编译文件"
	@echo "make help     - 显示此帮助信息"
	@echo "========================================="
	@echo ""
	@echo "第1阶段目标："
	@echo "  1. 编译驱动：make"
	@echo "  2. 部署驱动：make install"
	@echo "  3. 加载驱动：insmod /root/ds532/ds532_driver.ko"
	@echo "  4. 验证设备：ls -l /dev/ds532_fp"
	@echo ""

.PHONY: all modules test install clean help
