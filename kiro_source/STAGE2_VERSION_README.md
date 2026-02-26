# DS532驱动 Stage 2 版本说明

## 📋 版本概述

这是DS532驱动的Stage 2版本，只包含：
- **Stage 1**: 最小可运行驱动框架
- **Stage 2**: DS532协议封装功能

**不包含**：
- Stage 3: UART通信（需要硬件）
- Stage 4: GPIO中断（需要硬件）

## 🎯 适用场景

Stage 2版本适合：
1. **纯软件测试**: 无需硬件即可测试协议封装功能
2. **代码验证**: 验证数据包构造和校验和算法
3. **学习理解**: 理解DS532协议的实现细节
4. **逐步开发**: 在没有硬件的情况下先完成协议层

## 📁 文件说明

### 驱动文件
- `ds532_driver_stage2.c` - Stage 2版本驱动源码
- `ds532_driver.h` - 驱动头文件（共用）
- `Makefile.stage2` - Stage 2专用Makefile

### 测试文件
- `user/stage2_protocol_test.c` - 协议封装测试程序
- `STAGE2_TEST_GUIDE.md` - 测试指南

## 🚀 快速开始

### 1. 编译驱动

```bash
cd kiro_source
make -f Makefile.stage2
```

### 2. 编译测试程序

```bash
make -f Makefile.stage2 test
```

### 3. 运行测试（无需硬件）

```bash
cd user
./stage2_protocol_test
```

### 4. 部署到开发板（可选）

```bash
make -f Makefile.stage2 install

# 在开发板上
insmod /root/smart_lock/driver/ds532_driver_stage2.ko
dmesg | grep DS532
```

## ✅ Stage 2 功能列表

### 协议封装函数
- [x] `ds532_calculate_checksum()` - 校验和计算
- [x] `ds532_build_packet()` - 通用数据包构造
- [x] `ds532_build_vfypwd_packet()` - 验证密码命令
- [x] `ds532_build_genimg_packet()` - 采集图像命令
- [x] `ds532_build_img2tz_packet()` - 生成特征命令
- [x] `ds532_build_match_packet()` - 精确比对命令
- [x] `ds532_build_search_packet()` - 搜索指纹命令
- [x] `ds532_build_store_packet()` - 存储特征命令

### 响应包处理
- [x] `ds532_verify_packet()` - 验证响应包
- [x] `ds532_parse_response()` - 解析响应包

### 驱动基础功能
- [x] Platform驱动框架
- [x] 字符设备接口 (`/dev/ds532_fp`)
- [x] 设备独占访问控制
- [x] 文件操作（open/close/read/write/ioctl）
- [x] 并发控制（mutex）

## 🧪 测试方法

### 方法1: 用户空间测试程序（推荐）

```bash
cd user
./stage2_protocol_test
```

**优点**:
- 无需加载驱动
- 无需硬件
- 快速验证协议逻辑
- 详细的测试报告

### 方法2: 驱动内核日志测试

```bash
# 加载驱动
insmod ds532_driver_stage2.ko

# 打开设备触发IOCTL测试
cat /dev/ds532_fp &

# 查看内核日志
dmesg | grep DS532 | tail -30
```

**优点**:
- 验证驱动集成
- 测试内核空间实现
- 检查日志输出

## 📊 预期测试结果

### 用户空间测试
```
=================================================
    DS532驱动 Stage 2 协议封装测试程序
=================================================

📋 测试1: 验证密码命令包构造
VfyPwd包 (16 bytes): EF 01 FF FF FF FF 01 00 07 13 00 00 00 00 00 1B 
✅ 包头正确: 0xEF01
✅ 地址正确: 0xFFFFFFFF
✅ 包标识正确: 0x01
✅ 长度字段正确: 7
✅ 校验和正确: 0x001B
🎉 VfyPwd命令包 验证通过！

... (其他测试)

🎉 所有测试通过！Stage 2 协议封装功能正常
```

### 内核日志测试
```
[DS532] Probing DS532 driver (Stage 2)
[DS532] Allocated device number: 240:0
[DS532] Driver initialized successfully (Stage 2)
[DS532] Device node: /dev/ds532_fp
[DS532] Stage 2: Protocol encapsulation functions available
[DS532] Device open attempt
[DS532] Device opened successfully
[DS532] IOCTL: Init (VfyPwd)
[DS532] Built VfyPwd packet: 16 bytes
[DS532] VfyPwd: EF 01 FF FF FF FF 01 00 07 13 00 00 00 00 00 1B
```

## 🔄 与完整版本的区别

| 功能 | Stage 2版本 | 完整版本 |
|------|------------|----------|
| Platform驱动 | ✅ | ✅ |
| 字符设备 | ✅ | ✅ |
| 协议封装 | ✅ | ✅ |
| UART通信 | ❌ | ✅ |
| GPIO中断 | ❌ | ✅ |
| sysfs接口 | ❌ | ✅ |
| 硬件要求 | 无 | DS532模块 |

## 🎯 下一步

Stage 2测试通过后，可以：

1. **继续Stage 3开发**: 添加UART通信功能
2. **硬件准备**: 连接DS532模块到开发板
3. **集成测试**: 使用完整版本驱动进行硬件测试

## 📚 相关文档

- [STAGE2_SUMMARY.md](STAGE2_SUMMARY.md) - Stage 2完成总结
- [STAGE2_TEST_GUIDE.md](STAGE2_TEST_GUIDE.md) - 详细测试指南
- [PROGRESS.md](PROGRESS.md) - 项目进度跟踪

## 💡 使用建议

1. **先测试用户空间程序**: 快速验证协议逻辑
2. **再测试驱动**: 验证内核集成
3. **查看内核日志**: 理解数据包格式
4. **对比手册**: 验证协议实现正确性

---

**版本**: 0.2-stage2  
**日期**: 2026-02-26  
**状态**: 可用于纯软件测试