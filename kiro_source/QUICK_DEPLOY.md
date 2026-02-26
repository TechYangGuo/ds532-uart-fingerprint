# DS532驱动快速部署参考

## 🚀 一键部署

```bash
cd /share/project1/smart_face/fingerprint/kiro_source
make all-with-tests && make install
```

## 📦 部署文件

**目标路径**: `/root/smart_lock/driver/ds532/`

**核心文件**:
- `ds532_driver.ko` - 驱动模块
- `stage*_test*` - 测试程序

## 🔧 目标机快速测试

```bash
# 1. 连接
adb shell

# 2. 进入目录
cd /root/smart_lock/driver/ds532

# 3. 加载驱动
insmod ds532_driver.ko

# 4. 验证
ls -l /dev/ds532_fp
dmesg | grep DS532 | tail -10

# 5. 测试
./stage1_ioctl_test1_app

# 6. 卸载
rmmod ds532_driver
```

## 📋 Makefile命令速查

| 命令 | 说明 |
|------|------|
| `make` | 编译驱动 |
| `make test` | 编译测试程序 |
| `make all-with-tests` | 编译全部 |
| `make install` | 完整部署 |
| `make install-quick` | 快速部署 |
| `make clean` | 清理本地 |
| `make clean-target` | 清理目标机 |
| `make help` | 显示帮助 |

## 🔄 快速更新流程

```bash
# 开发主机
make clean
make all-with-tests
make install-quick

# 目标机
rmmod ds532_driver
insmod /root/smart_lock/driver/ds532/ds532_driver.ko
```

## ⚠️ 故障排查

```bash
# 检查ADB
adb devices

# 检查文件
adb shell "ls -lh /root/smart_lock/driver/ds532/"

# 查看日志
adb shell "dmesg | grep DS532"

# 重新连接
adb disconnect && adb connect 192.168.1.xxx:5555
```