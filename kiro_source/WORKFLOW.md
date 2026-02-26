# DS532驱动开发工作流程

## 📋 完整开发流程

```
┌─────────────────────────────────────────────────────────────┐
│                    DS532驱动开发工作流程                      │
└─────────────────────────────────────────────────────────────┘

1️⃣ 开发阶段 (开发主机)
   ├── 编写代码
   │   ├── ds532_driver.c (驱动源码)
   │   ├── ds532_driver.h (驱动头文件)
   │   └── user/*.c (测试程序)
   │
   ├── 编译
   │   └── make all-with-tests
   │
   └── 本地验证
       └── 检查编译输出

2️⃣ 部署阶段 (开发主机 → 目标机)
   ├── 快速部署
   │   └── make install-quick
   │
   └── 完整部署
       └── make install

3️⃣ 测试阶段 (目标机)
   ├── 加载驱动
   │   └── insmod ds532_driver.ko
   │
   ├── 验证设备
   │   ├── ls -l /dev/ds532_fp
   │   └── dmesg | grep DS532
   │
   ├── 运行测试
   │   ├── ./stage1_ioctl_test1_app
   │   ├── ./stage2_protocol_test
   │   ├── ./stage3_uart_test
   │   └── ./stage4_gpio_test
   │
   └── 卸载驱动
       └── rmmod ds532_driver

4️⃣ 调试阶段 (根据需要)
   ├── 查看日志
   │   └── dmesg | grep DS532
   │
   ├── 修改代码
   │   └── 返回步骤1
   │
   └── 重新部署
       └── make clean && make all-with-tests && make install-quick

5️⃣ 提交阶段 (版本控制)
   ├── Git提交
   │   ├── git add .
   │   ├── git commit -m "Stage X: 功能描述"
   │   └── git tag vX.X-stageX
   │
   └── 更新文档
       └── 更新PROGRESS.md
```

---

## 🔄 日常开发循环

### 快速迭代流程

```bash
# 1. 修改代码
vim ds532_driver.c

# 2. 编译
make clean && make

# 3. 快速部署
make install-quick

# 4. 在目标机测试
adb shell "cd /root/smart_lock/driver/ds532 && rmmod ds532_driver; insmod ds532_driver.ko"

# 5. 查看日志
adb shell "dmesg | grep DS532 | tail -20"
```

### 完整测试流程

```bash
# 1. 完整编译
make clean
make all-with-tests

# 2. 完整部署
make install

# 3. 连接目标机
adb shell

# 4. 进入目录
cd /root/smart_lock/driver/ds532

# 5. 加载驱动
insmod ds532_driver.ko

# 6. 运行所有测试
./stage1_ioctl_test1_app
./stage2_protocol_test
./stage3_uart_test
./stage4_gpio_test

# 7. 查看结果
dmesg | grep DS532

# 8. 卸载驱动
rmmod ds532_driver
```

---

## 📊 阶段开发流程

### Stage 1: 基础框架

```bash
# 开发
vim ds532_driver.c  # 实现Platform驱动框架

# 编译
make

# 部署
make install-quick

# 测试
adb shell "cd /root/smart_lock/driver/ds532 && insmod ds532_driver.ko"
adb shell "./stage1_ioctl_test1_app"

# 验证
adb shell "dmesg | grep DS532"

# 提交
git add .
git commit -m "Stage 1: 最小可运行驱动框架"
git tag v0.1-stage1
```

### Stage 2: 协议封装

```bash
# 开发
vim ds532_driver.c  # 添加协议封装函数

# 编译
make clean && make all-with-tests

# 部署
make install-quick

# 测试（无需硬件）
adb shell "./stage2_protocol_test"

# 提交
git add .
git commit -m "Stage 2: DS532协议封装"
git tag v0.2-stage2
```

### Stage 3: UART通信

```bash
# 开发
vim ds532_driver.c  # 添加UART通信功能

# 编译
make clean && make all-with-tests

# 部署
make install

# 测试（需要硬件）
adb shell "cd /root/smart_lock/driver/ds532 && insmod ds532_driver.ko"
adb shell "./stage3_uart_test"

# 查看日志
adb shell "dmesg | grep DS532 | tail -30"

# 提交
git add .
git commit -m "Stage 3: UART通信实现"
git tag v0.3-stage3
```

### Stage 4: GPIO中断

```bash
# 开发
vim ds532_driver.c  # 添加GPIO中断处理

# 编译
make clean && make all-with-tests

# 部署
make install

# 测试（需要硬件）
adb shell "cd /root/smart_lock/driver/ds532 && insmod ds532_driver.ko"
adb shell "./stage4_gpio_test"

# 验证sysfs
adb shell "cat /sys/class/ds532/ds532_fp/touch_count"

# 提交
git add .
git commit -m "Stage 4: GPIO中断处理"
git tag v0.4-stage4
```

---

## 🛠️ Makefile命令参考

### 编译命令

| 命令 | 说明 | 输出 |
|------|------|------|
| `make` | 编译驱动模块 | `ds532_driver.ko` |
| `make modules` | 编译驱动模块 | `ds532_driver.ko` |
| `make test` | 编译测试程序 | `user/stage*` |
| `make all-with-tests` | 编译驱动和测试 | 全部文件 |

### 部署命令

| 命令 | 说明 | 部署内容 |
|------|------|----------|
| `make install` | 完整部署 | 驱动+测试+文档 |
| `make install-quick` | 快速部署 | 驱动+测试 |

### 清理命令

| 命令 | 说明 | 清理范围 |
|------|------|----------|
| `make clean` | 清理本地 | 编译文件 |
| `make clean-target` | 清理目标机 | 部署文件 |

### 帮助命令

| 命令 | 说明 |
|------|------|
| `make help` | 显示帮助信息 |

---

## 🔍 调试技巧

### 1. 查看驱动日志

```bash
# 实时查看日志
adb shell "dmesg -w | grep DS532"

# 查看最近日志
adb shell "dmesg | grep DS532 | tail -30"

# 清空日志后重新测试
adb shell "dmesg -c > /dev/null"
adb shell "insmod /root/smart_lock/driver/ds532/ds532_driver.ko"
adb shell "dmesg | grep DS532"
```

### 2. 检查设备状态

```bash
# 检查设备节点
adb shell "ls -l /dev/ds532_fp"

# 检查驱动模块
adb shell "lsmod | grep ds532"

# 检查sysfs接口
adb shell "ls -l /sys/class/ds532/"
```

### 3. 检查UART状态

```bash
# 检查UART设备
adb shell "ls -l /dev/ttymxc5"

# 检查UART配置
adb shell "stty -F /dev/ttymxc5"
```

### 4. 检查GPIO状态

```bash
# 检查中断
adb shell "cat /proc/interrupts | grep ds532"

# 检查GPIO
adb shell "cat /sys/kernel/debug/gpio"
```

### 5. 性能分析

```bash
# 测试命令执行时间
adb shell "time ./stage3_uart_test"

# 查看系统资源
adb shell "top -n 1 | grep ds532"
```

---

## 📝 版本管理

### Git工作流

```bash
# 1. 创建功能分支
git checkout -b feature/stage-X

# 2. 开发和提交
git add .
git commit -m "实现Stage X功能"

# 3. 合并到主分支
git checkout main
git merge feature/stage-X

# 4. 创建标签
git tag -a v0.X-stageX -m "Stage X: 功能描述"

# 5. 推送
git push origin main --tags
```

### 版本号规则

- `v0.1-stage1` - Stage 1完成
- `v0.2-stage2` - Stage 2完成
- `v0.3-stage3` - Stage 3完成
- `v0.4-stage4` - Stage 4完成
- `v1.0` - 正式版本

---

## 🎯 最佳实践

### 开发建议

1. **小步快跑**: 每完成一个小功能就编译测试
2. **频繁提交**: 每个功能点都提交一次
3. **详细日志**: 在关键位置添加调试日志
4. **错误处理**: 每个函数都要有完整的错误处理
5. **代码审查**: 提交前检查代码规范

### 测试建议

1. **分阶段测试**: 按Stage顺序逐个测试
2. **无硬件测试**: 优先测试Stage 1和Stage 2
3. **硬件测试**: 准备好硬件后测试Stage 3和Stage 4
4. **回归测试**: 修改代码后重新运行所有测试
5. **日志分析**: 仔细分析内核日志中的错误信息

### 部署建议

1. **快速迭代**: 开发阶段使用 `make install-quick`
2. **完整测试**: 测试阶段使用 `make install`
3. **清理环境**: 遇到问题时先 `make clean-target`
4. **版本管理**: 重要版本打标签
5. **文档同步**: 及时更新PROGRESS.md

---

## 📚 相关文档

- `README.md` - 项目说明
- `DEPLOYMENT_GUIDE.md` - 详细部署指南
- `QUICK_DEPLOY.md` - 快速部署参考
- `PROGRESS.md` - 开发进度跟踪
- `STAGE*_TEST_GUIDE.md` - 各阶段测试指南

---

## 🆘 常见问题

### Q1: 编译失败怎么办？

```bash
# 1. 清理后重新编译
make clean
make

# 2. 检查内核路径
ls /home/book/Workspace/wds/sdk/imx6ull/100ask_imx6ull-sdk/Linux-4.9.88

# 3. 检查工具链
which arm-buildroot-linux-gnueabihf-gcc
```

### Q2: 部署失败怎么办？

```bash
# 1. 检查ADB连接
adb devices

# 2. 重新连接
adb disconnect
adb connect 192.168.1.xxx:5555

# 3. 检查目标目录
adb shell "mkdir -p /root/smart_lock/driver/ds532"
```

### Q3: 驱动加载失败怎么办？

```bash
# 1. 查看详细错误
dmesg | tail -50

# 2. 检查内核版本
uname -r
modinfo ds532_driver.ko

# 3. 检查依赖
lsmod
```

### Q4: 测试程序无法运行？

```bash
# 1. 检查权限
chmod +x /root/smart_lock/driver/ds532/stage*

# 2. 检查设备节点
ls -l /dev/ds532_fp

# 3. 检查驱动是否加载
lsmod | grep ds532
```

---

**更新时间**: 2026-02-26  
**适用版本**: v0.1 - v0.4  
**维护者**: 开发团队
