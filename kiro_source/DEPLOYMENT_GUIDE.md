# DS532驱动部署指南

## 📋 部署架构

```
开发主机 (Samba服务器)                    目标机 (IMX6ULL)
┌─────────────────────────┐              ┌──────────────────────────┐
│ /share/project1/        │              │ /root/smart_lock/        │
│ smart_face/fingerprint/ │              │ driver/ds532/            │
│                         │              │                          │
│ kiro_source/            │   ADB Push   │ ├── ds532_driver.ko      │
│ ├── ds532_driver.c      │ ──────────>  │ ├── ds532_fingerprint.dts│
│ ├── ds532_driver.h      │              │ ├── stage1_ioctl_test1_app│
│ ├── Makefile            │              │ ├── stage2_protocol_test │
│ └── user/               │              │ ├── stage3_uart_test     │
│     ├── stage1_*.c      │              │ ├── stage4_gpio_test     │
│     ├── stage2_*.c      │              │ └── *.md (文档)          │
│     ├── stage3_*.c      │              │                          │
│     └── stage4_*.c      │              └──────────────────────────┘
└─────────────────────────┘
```

## 📦 部署文件清单

### 必需文件（核心功能）
| 文件名 | 大小 | 说明 | 优先级 |
|--------|------|------|--------|
| `ds532_driver.ko` | ~50KB | 驱动内核模块 | ⭐⭐⭐ 必需 |
| `ds532_fingerprint.dts` | ~2KB | 设备树源文件 | ⭐⭐⭐ 必需 |

### 测试程序（功能验证）
| 文件名 | 大小 | 说明 | 优先级 |
|--------|------|------|--------|
| `stage1_ioctl_test1_app` | ~20KB | Stage 1基础测试 | ⭐⭐⭐ 推荐 |
| `stage2_protocol_test` | ~25KB | Stage 2协议测试 | ⭐⭐ 可选 |
| `stage3_uart_test` | ~30KB | Stage 3 UART测试 | ⭐⭐⭐ 推荐 |
| `stage4_gpio_test` | ~25KB | Stage 4 GPIO测试 | ⭐⭐ 可选 |

### 文档文件（参考资料）
| 文件名 | 大小 | 说明 | 优先级 |
|--------|------|------|--------|
| `README.md` | ~5KB | 项目说明 | ⭐ 可选 |
| `STAGE1_TEST_GUIDE.md` | ~8KB | Stage 1测试指南 | ⭐ 可选 |
| `STAGE2_TEST_GUIDE.md` | ~10KB | Stage 2测试指南 | ⭐ 可选 |
| `STAGE3_TEST_GUIDE.md` | ~12KB | Stage 3测试指南 | ⭐ 可选 |
| `STAGE4_TEST_GUIDE.md` | ~10KB | Stage 4测试指南 | ⭐ 可选 |

## 🚀 部署方法

### 方法1: 完整部署（推荐）

```bash
# 在开发主机上
cd /share/project1/smart_face/fingerprint/kiro_source

# 编译驱动和测试程序
make all-with-tests

# 完整部署（驱动+测试+文档）
make install
```

**部署内容**:
- ✅ 驱动模块
- ✅ 设备树文件
- ✅ 所有测试程序
- ✅ 所有文档

### 方法2: 快速部署（仅核心文件）

```bash
# 编译
make all-with-tests

# 快速部署（仅驱动+测试）
make install-quick
```

**部署内容**:
- ✅ 驱动模块
- ✅ 测试程序
- ❌ 文档（不部署）

### 方法3: 手动部署（自定义）

```bash
# 创建目标目录
adb shell "mkdir -p /root/smart_lock/driver/ds532"

# 部署驱动
adb push ds532_driver.ko /root/smart_lock/driver/ds532/

# 部署测试程序（按需选择）
adb push user/stage1_ioctl_test1_app /root/smart_lock/driver/ds532/
adb push user/stage3_uart_test /root/smart_lock/driver/ds532/

# 部署设备树
adb push ds532_fingerprint.dts /root/smart_lock/driver/ds532/
```

## 📋 部署后验证

### 1. 检查文件是否部署成功

```bash
# 在开发主机上
adb shell "ls -lh /root/smart_lock/driver/ds532/"
```

**预期输出**:
```
-rw-r--r-- 1 root root  50K Feb 26 10:00 ds532_driver.ko
-rw-r--r-- 1 root root   2K Feb 26 10:00 ds532_fingerprint.dts
-rwxr-xr-x 1 root root  20K Feb 26 10:00 stage1_ioctl_test1_app
-rwxr-xr-x 1 root root  30K Feb 26 10:00 stage3_uart_test
...
```

### 2. 检查文件权限

```bash
# 确保测试程序有执行权限
adb shell "chmod +x /root/smart_lock/driver/ds532/stage*"
```

### 3. 验证ADB连接

```bash
# 检查ADB连接状态
adb devices

# 应该看到设备列表
# List of devices attached
# 192.168.1.xxx:5555    device
```

## 🔧 目标机使用流程

### 1. 连接到目标机

```bash
adb shell
```

### 2. 进入驱动目录

```bash
cd /root/smart_lock/driver/ds532
ls -lh
```

### 3. 加载驱动

```bash
insmod ds532_driver.ko
```

### 4. 验证驱动加载

```bash
# 检查设备节点
ls -l /dev/ds532_fp

# 查看内核日志
dmesg | grep DS532 | tail -20

# 检查驱动模块
lsmod | grep ds532
```

### 5. 运行测试程序

```bash
# Stage 1测试（基础功能）
./stage1_ioctl_test1_app

# Stage 3测试（UART通信，需要硬件）
./stage3_uart_test

# Stage 4测试（GPIO中断，需要硬件）
./stage4_gpio_test
```

### 6. 卸载驱动

```bash
rmmod ds532_driver

# 验证卸载
lsmod | grep ds532
```

## 🔄 更新部署

### 更新驱动模块

```bash
# 在开发主机上
cd kiro_source
make clean
make
make install-quick

# 在目标机上
rmmod ds532_driver
insmod /root/smart_lock/driver/ds532/ds532_driver.ko
```

### 更新测试程序

```bash
# 在开发主机上
cd kiro_source
make test
adb push user/stage3_uart_test /root/smart_lock/driver/ds532/
```

## 🧹 清理操作

### 清理开发主机编译文件

```bash
cd kiro_source
make clean
```

### 清理目标机部署文件

```bash
# 方法1: 使用Makefile
make clean-target

# 方法2: 手动清理
adb shell "rm -rf /root/smart_lock/driver/ds532"
```

## ⚠️ 常见问题

### 问题1: ADB连接失败

```bash
# 检查网络连接
ping 192.168.1.xxx

# 重新连接ADB
adb disconnect
adb connect 192.168.1.xxx:5555
```

### 问题2: 文件推送失败

```bash
# 检查目标目录是否存在
adb shell "mkdir -p /root/smart_lock/driver/ds532"

# 检查磁盘空间
adb shell "df -h /root"
```

### 问题3: 驱动加载失败

```bash
# 查看详细错误信息
dmesg | tail -50

# 检查内核版本匹配
uname -r
modinfo ds532_driver.ko
```

### 问题4: 测试程序无法执行

```bash
# 添加执行权限
chmod +x /root/smart_lock/driver/ds532/stage*

# 检查文件完整性
ls -lh /root/smart_lock/driver/ds532/
```

## 📊 部署检查清单

- [ ] ADB连接正常
- [ ] 目标目录已创建
- [ ] 驱动模块已部署
- [ ] 测试程序已部署
- [ ] 测试程序有执行权限
- [ ] 驱动可以正常加载
- [ ] 设备节点已创建
- [ ] 测试程序可以运行

## 🎯 最佳实践

1. **开发阶段**: 使用 `make install-quick` 快速部署
2. **测试阶段**: 使用 `make install` 完整部署
3. **生产阶段**: 只部署必需文件（驱动模块）
4. **版本管理**: 使用Git标签标记每次部署的版本
5. **日志记录**: 保存每次部署的内核日志用于问题追踪

---

**目标路径**: `/root/smart_lock/driver/ds532/`  
**部署方式**: ADB Push  
**更新日期**: 2026-02-26