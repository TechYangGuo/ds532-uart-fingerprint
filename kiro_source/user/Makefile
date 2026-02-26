# DS532驱动测试程序 Makefile
# 用于编译用户空间测试程序
# 目标平台: IMX6ULL (ARM)

# 交叉编译工具链（与主Makefile保持一致）
CC = /home/book/Workspace/wds/sdk/imx6ull/100ask_imx6ull-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/arm-buildroot-linux-gnueabihf-gcc
CFLAGS = -Wall -Wextra -std=c99

# 目标程序
TARGETS = stage1_ioctl_test1_app stage2_protocol_test stage3_uart_test stage4_gpio_test

# 默认目标
all: $(TARGETS)

# 编译规则
stage1_ioctl_test1_app: stage1_ioctl_test1.c
	$(CC) $(CFLAGS) -o $@ $<

stage2_protocol_test: stage2_protocol_test.c
	$(CC) $(CFLAGS) -o $@ $<

stage3_uart_test: stage3_uart_test.c
	$(CC) $(CFLAGS) -o $@ $<

stage4_gpio_test: stage4_gpio_test.c
	$(CC) $(CFLAGS) -o $@ $<

# 清理
clean:
	rm -f $(TARGETS)

# 测试目标
test-stage1: stage1_ioctl_test1_app
	@echo "=== Running Stage 1 Test ==="
	./stage1_ioctl_test1_app

test-stage2: stage2_protocol_test
	@echo "=== Running Stage 2 Test ==="
	./stage2_protocol_test

test-stage3: stage3_uart_test
	@echo "=== Running Stage 3 Test ==="
	./stage3_uart_test

test-stage4: stage4_gpio_test
	@echo "=== Running Stage 4 Test ==="
	./stage4_gpio_test

# 运行所有测试
test-all: $(TARGETS)
	@echo "=== Running All Tests ==="
	@echo "Stage 1: Basic Driver Framework"
	./stage1_ioctl_test1_app
	@echo ""
	@echo "Stage 2: Protocol Encapsulation"
	./stage2_protocol_test
	@echo ""
	@echo "Stage 3: UART Communication (需要硬件)"
	@echo "请手动运行: ./stage3_uart_test"
	@echo ""
	@echo "Stage 4: GPIO Interrupt (需要硬件)"
	@echo "请手动运行: ./stage4_gpio_test"

# 部署到开发板
deploy: $(TARGETS)
	@echo "部署测试程序到开发板..."
	adb push stage1_ioctl_test1_app /root/smart_lock/driver/
	adb push stage2_protocol_test /root/smart_lock/driver/
	adb push stage3_uart_test /root/smart_lock/driver/
	adb push stage4_gpio_test /root/smart_lock/driver/
	@echo "部署完成！"

# 安装（可选）
install: $(TARGETS)
	@echo "Installing test programs..."
	# 这里可以添加安装逻辑

.PHONY: all clean install test-stage1 test-stage2 test-stage3 test-stage4 test-all deploy