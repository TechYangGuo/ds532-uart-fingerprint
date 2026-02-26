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

# 编译测试程序
test:
	@echo "========================================="
	@echo "编译测试程序..."
	@echo "========================================="
	@cd user && $(MAKE) all
	@echo ""
	@echo "测试程序编译完成！"
	@echo ""
	@ls -lh user/stage*_test* user/stage*_app 2>/dev/null || echo "查看user目录"
	@echo ""

# 编译所有（驱动+测试程序）
all-with-tests: modules test
	@echo "========================================="
	@echo "驱动和测试程序全部编译完成！"
	@echo "========================================="

# 部署到开发板
install: modules
	@echo "========================================="
	@echo "部署DS532驱动到目标机..."
	@echo "========================================="
	@echo "目标路径: /root/smart_lock/driver/ds532"
	@echo ""
	@echo "创建目标目录..."
	adb shell "mkdir -p /root/smart_lock/driver/ds532"
	@echo ""
	@echo "部署驱动模块..."
	adb push ds532_driver.ko /root/smart_lock/driver/ds532/
	@echo ""
	@echo "部署设备树文件..."
	adb push ds532_fingerprint.dts /root/smart_lock/driver/ds532/
	@echo ""
	@echo "部署测试程序..."
	adb push user/stage1_ioctl_test1_app /root/smart_lock/driver/ds532/ 2>/dev/null || echo "  stage1测试程序未编译"
	adb push user/stage2_protocol_test /root/smart_lock/driver/ds532/ 2>/dev/null || echo "  stage2测试程序未编译"
	adb push user/stage3_uart_test /root/smart_lock/driver/ds532/ 2>/dev/null || echo "  stage3测试程序未编译"
	adb push user/stage4_gpio_test /root/smart_lock/driver/ds532/ 2>/dev/null || echo "  stage4测试程序未编译"
	@echo ""
	@echo "部署文档..."
	adb push README.md /root/smart_lock/driver/ds532/ 2>/dev/null || echo "  README未找到"
	adb push STAGE1_TEST_GUIDE.md /root/smart_lock/driver/ds532/ 2>/dev/null || echo "  STAGE1_TEST_GUIDE未找到"
	adb push STAGE2_TEST_GUIDE.md /root/smart_lock/driver/ds532/ 2>/dev/null || echo "  STAGE2_TEST_GUIDE未找到"
	adb push STAGE3_TEST_GUIDE.md /root/smart_lock/driver/ds532/ 2>/dev/null || echo "  STAGE3_TEST_GUIDE未找到"
	adb push STAGE4_TEST_GUIDE.md /root/smart_lock/driver/ds532/ 2>/dev/null || echo "  STAGE4_TEST_GUIDE未找到"
	@echo ""
	@echo "========================================="
	@echo "部署完成！"
	@echo "========================================="
	@echo ""
	@echo "目标机文件列表："
	adb shell "ls -lh /root/smart_lock/driver/ds532/"
	@echo ""
	@echo "📋 使用说明："
	@echo "----------------------------------------"
	@echo "1. 加载驱动："
	@echo "   cd /root/smart_lock/driver/ds532"
	@echo "   insmod ds532_driver.ko"
	@echo ""
	@echo "2. 验证设备节点："
	@echo "   ls -l /dev/ds532_fp"
	@echo ""
	@echo "3. 查看内核日志："
	@echo "   dmesg | grep DS532"
	@echo ""
	@echo "4. 运行测试程序："
	@echo "   ./stage1_ioctl_test1_app    # Stage 1测试"
	@echo "   ./stage2_protocol_test       # Stage 2测试"
	@echo "   ./stage3_uart_test           # Stage 3测试（需要硬件）"
	@echo "   ./stage4_gpio_test           # Stage 4测试（需要硬件）"
	@echo ""
	@echo "5. 卸载驱动："
	@echo "   rmmod ds532_driver"
	@echo "========================================="
	@echo ""

# 快速部署（仅驱动和测试程序）
install-quick: modules
	@echo "快速部署（仅驱动和测试程序）..."
	adb shell "mkdir -p /root/smart_lock/driver/ds532"
	adb push ds532_driver.ko /root/smart_lock/driver/ds532/
	adb push user/stage*_test* /root/smart_lock/driver/ds532/ 2>/dev/null || true
	adb push user/stage*_app /root/smart_lock/driver/ds532/ 2>/dev/null || true
	@echo "快速部署完成！"
	@echo ""

# 清理编译文件
clean:
	@echo "清理编译文件..."
	make $(ARCH_ARGS) -C $(KDIR) M=$(PWD) clean
	@cd user && $(MAKE) clean
	@echo "清理完成！"

# 清理目标机上的文件
clean-target:
	@echo "清理目标机上的驱动文件..."
	adb shell "rm -rf /root/smart_lock/driver/ds532"
	@echo "目标机清理完成！"

# 帮助信息
help:
	@echo "DS532驱动 Makefile 使用说明"
	@echo "========================================="
	@echo "编译命令："
	@echo "  make              - 编译驱动模块"
	@echo "  make modules      - 编译驱动模块"
	@echo "  make test         - 编译测试程序"
	@echo "  make all-with-tests - 编译驱动和测试程序"
	@echo ""
	@echo "部署命令："
	@echo "  make install      - 完整部署（驱动+测试+文档）"
	@echo "  make install-quick - 快速部署（仅驱动+测试）"
	@echo ""
	@echo "清理命令："
	@echo "  make clean        - 清理本地编译文件"
	@echo "  make clean-target - 清理目标机文件"
	@echo ""
	@echo "其他命令："
	@echo "  make help         - 显示此帮助信息"
	@echo "========================================="
	@echo ""
	@echo "📋 完整开发流程："
	@echo "----------------------------------------"
	@echo "1. 编译驱动和测试程序："
	@echo "   make all-with-tests"
	@echo ""
	@echo "2. 部署到目标机："
	@echo "   make install"
	@echo ""
	@echo "3. 在目标机上测试："
	@echo "   adb shell"
	@echo "   cd /root/smart_lock/driver/ds532"
	@echo "   insmod ds532_driver.ko"
	@echo "   ./stage1_ioctl_test1_app"
	@echo ""
	@echo "4. 查看日志："
	@echo "   dmesg | grep DS532"
	@echo ""
	@echo "5. 卸载驱动："
	@echo "   rmmod ds532_driver"
	@echo "========================================="
	@echo ""
	@echo "📁 目标机部署路径："
	@echo "   /root/smart_lock/driver/ds532/"
	@echo ""
	@echo "📦 部署文件列表："
	@echo "   - ds532_driver.ko           (驱动模块)"
	@echo "   - ds532_fingerprint.dts     (设备树)"
	@echo "   - stage1_ioctl_test1_app    (Stage 1测试)"
	@echo "   - stage2_protocol_test      (Stage 2测试)"
	@echo "   - stage3_uart_test          (Stage 3测试)"
	@echo "   - stage4_gpio_test          (Stage 4测试)"
	@echo "   - *.md                      (测试指南文档)"
	@echo ""

.PHONY: all modules test all-with-tests install install-quick clean clean-target help
