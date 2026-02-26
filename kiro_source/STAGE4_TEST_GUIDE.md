# Stage 4 GPIO中断测试指南

## 📋 测试概述

Stage 4 测试验证GPIO中断功能，包括：
- GPIO中断检测
- sysfs接口访问
- 触摸事件统计
- 触摸时间戳记录

## ⚠️ 重要前提条件

### 硬件要求
1. **DS532指纹模块**: 正确连接到开发板
2. **GPIO连接**: 触摸引脚连接到GPIO4_IO21
3. **中断配置**: 设备树中正确配置GPIO中断
4. **传感器工作**: DS532传感器能正常检测触摸

### 软件要求
1. **驱动加载**: DS532驱动已正确加载（包含Stage 4代码）
2. **sysfs接口**: 相关sysfs节点已创建
3. **中断注册**: GPIO中断已成功注册

## 🚀 快速开始

### 1. 准备环境

```bash
# 加载驱动（确保包含Stage 4代码）
insmod /root/smart_lock/driver/ds532_driver.ko

# 检查设备节点
ls -l /dev/ds532_fp

# 检查sysfs接口
ls -l /sys/class/ds532/ds532_fp/touch_count
ls -l /sys/class/ds532/ds532_fp/last_touch_time

# 检查中断注册
cat /proc/interrupts | grep ds532

# 查看内核日志
dmesg | grep DS532 | tail -10
```

### 2. 编译测试程序

```bash
cd kiro_source/user
make stage4_gpio_test
```

### 3. 运行测试

```bash
./stage4_gpio_test
```

## 📊 测试内容详解

### 测试1: sysfs接口访问
- **目的**: 验证sysfs属性文件的可访问性
- **测试项**:
  - 读取 touch_count 属性
  - 读取 last_touch_time 属性
  - 验证数据格式正确性

**sysfs路径**:
```
/sys/class/ds532/ds532_fp/touch_count      # 触摸计数
/sys/class/ds532/ds532_fp/last_touch_time  # 最后触摸时间(纳秒)
```

### 测试2: 触摸事件监控
- **目的**: 验证GPIO中断的实时检测能力
- **测试流程**:
  1. 记录初始触摸计数
  2. 监控30秒
  3. 检测触摸事件变化
  4. 统计事件数量

**用户操作**: 在30秒内多次触摸指纹传感器

### 测试3: 触摸响应时间测试
- **目的**: 测量GPIO中断的响应时间
- **测试流程**:
  1. 等待5个触摸事件
  2. 记录每次触摸的时间戳
  3. 计算触摸间隔
  4. 分析响应性能

### 测试4: 设备打开时的GPIO状态
- **目的**: 验证设备打开状态下GPIO功能正常
- **测试流程**:
  1. 记录设备打开前状态
  2. 打开设备文件
  3. 等待触摸事件
  4. 记录设备打开后状态
  5. 比较前后差异

## ✅ 成功标准

### 基本功能
- [x] sysfs接口可正常访问
- [x] 触摸事件能被检测到
- [x] 触摸计数正确递增
- [x] 时间戳正确更新
- [x] 中断响应及时

### 性能指标
- [x] 中断响应时间 < 10ms
- [x] 触摸检测准确率 > 95%
- [x] 无误触发或漏触发
- [x] 系统稳定性良好

## 🔍 预期输出示例

```
=================================================
    DS532驱动 Stage 4 GPIO中断测试程序
=================================================

📋 测试前检查：
   1. 确保驱动已加载: insmod ds532_driver.ko
   2. 确保设备节点存在: ls -l /dev/ds532_fp
   3. 确保sysfs接口存在:
      ls -l /sys/class/ds532/ds532_fp/touch_count
      ls -l /sys/class/ds532/ds532_fp/last_touch_time
   4. 确保GPIO连接正常: 触摸引脚连接到GPIO4_IO21
   5. 确保指纹传感器工作正常

按回车键继续...

📋 测试1: sysfs接口访问
✅ touch_count: 0
✅ last_touch_time: 0 ns

📋 测试2: 触摸事件监控
💡 请在接下来的30秒内触摸指纹传感器...
初始触摸计数: 0
初始触摸时间: 0 ns

开始监控...
🔔 检测到触摸事件 #1: count=1, time=1234567890123456789 ns
🔔 检测到触摸事件 #2: count=2, time=1234567891234567890 ns
🔔 检测到触摸事件 #3: count=3, time=1234567892345678901 ns
⏰ 监控中... 5/30 秒
🔔 检测到触摸事件 #4: count=4, time=1234567893456789012 ns
⏰ 监控中... 10/30 秒
⏰ 监控中... 15/30 秒
🔔 检测到触摸事件 #5: count=5, time=1234567894567890123 ns
⏰ 监控中... 20/30 秒
⏰ 监控中... 25/30 秒
⏰ 监控中... 30/30 秒

监控结束
总共检测到 5 个触摸事件
✅ GPIO中断功能正常

📋 测试3: 触摸响应时间测试
💡 请快速触摸指纹传感器5次...
等待触摸事件...
触摸 #1: time=1234567895678901234 ns
触摸 #2: time=1234567896789012345 ns
触摸 #3: time=1234567897890123456 ns
触摸 #4: time=1234567898901234567 ns
触摸 #5: time=1234567899012345678 ns

📊 响应时间分析：
触摸间隔 1-2: 1110.11 ms
触摸间隔 2-3: 1101.11 ms
触摸间隔 3-4: 1011.11 ms
触摸间隔 4-5: 1111.11 ms
✅ 响应时间测试完成

📋 测试4: 设备打开时的GPIO状态
设备打开前: count=10, time=1234567899012345678
✅ 设备打开成功
💡 设备打开状态下，请触摸传感器...
设备打开后: count=12, time=1234567900123456789
✅ 设备关闭
✅ 检测到 2 个新的触摸事件

=================================================
                  测试结果汇总
=================================================
🎉 所有测试通过！Stage 4 GPIO中断功能正常

✅ 已验证功能：
   - sysfs接口访问
   - 触摸事件检测
   - 触摸计数统计
   - 触摸时间戳记录
   - GPIO中断响应
   - 设备状态下的GPIO功能

🚀 Stage 4 测试完成，可以进行 Stage 5 开发
```

## 🔧 故障排除

### sysfs接口不存在
```bash
# 检查驱动加载
lsmod | grep ds532

# 检查设备类创建
ls -l /sys/class/ds532/

# 查看内核日志
dmesg | grep DS532 | grep sysfs
```

### 触摸事件检测不到
```bash
# 检查GPIO配置
cat /sys/kernel/debug/gpio

# 检查中断注册
cat /proc/interrupts | grep ds532

# 检查设备树配置
cat /proc/device-tree/ds532-fingerprint/touch-irq-gpios

# 查看中断统计
watch -n 1 'cat /proc/interrupts | grep ds532'
```

### GPIO中断问题
```bash
# 检查GPIO引脚状态
echo 117 > /sys/class/gpio/export  # GPIO4_IO21 = 4*32+21 = 117
cat /sys/class/gpio/gpio117/value
echo 117 > /sys/class/gpio/unexport

# 检查中断触发
dmesg | grep -i interrupt | grep gpio
```

### 硬件连接检查
1. **电源检查**: 确保DS532模块3.3V供电正常
2. **信号检查**: 使用万用表测试触摸引脚电平变化
3. **接线检查**: 确认触摸引脚正确连接到GPIO4_IO21

## 📚 相关文档

- [GPIO子系统文档](https://www.kernel.org/doc/Documentation/gpio/)
- [设备树GPIO配置](https://www.kernel.org/doc/Documentation/devicetree/bindings/gpio/)
- [Stage 3 测试指南](STAGE3_TEST_GUIDE.md)

## 🎯 下一步

Stage 4 测试通过后，可以进行：
1. Stage 5 完整功能测试
2. 集成所有功能的综合测试
3. 性能和稳定性测试

---

**测试程序**: `stage4_gpio_test.c`  
**编译命令**: `make stage4_gpio_test`  
**运行命令**: `./stage4_gpio_test`  
**硬件要求**: DS532模块 + GPIO4_IO21连接