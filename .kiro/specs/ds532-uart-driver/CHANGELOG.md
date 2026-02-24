# DS532 UART驱动规范变更记录

## 2026-02-23 - 需求文档重大更新

### 修改原因
根据用户反馈和实际硬件配置，对需求文档进行了重要修正。

### 主要变更

#### 1. UART设备节点修正
- **修改前**: UART6 对应 `/dev/ttymxc4`
- **修改后**: UART6 对应 `/dev/ttymxc5`
- **影响范围**: 
  - requirements.md 术语表
  - fingerprint_test.c 设备路径
  - README.md 文档说明

#### 2. GPIO功能重新定义
- **修改前**: GPIO_Wake - 用于唤醒指纹模块（输出模式）
- **修改后**: GPIO_Touch_IRQ - 接收指纹模块触摸中断信号（输入模式）
- **功能说明**: 
  - 当用户触摸指纹模块时，模块产生电平变化
  - SOC通过GPIO中断接收此信号
  - 驱动需要注册中断处理函数响应触摸事件
- **影响范围**:
  - requirements.md 术语表
  - requirements.md 需求1（初始化）
  - requirements.md 需求5（完全重写为GPIO中断处理）
  - requirements.md 需求8（设备树配置）

#### 3. 设备树属性名称变更
- **修改前**: `wake-gpios`
- **修改后**: `touch-irq-gpios`
- **影响范围**:
  - fingerprint.dts
  - requirements.md 需求8

#### 4. 并发控制模式明确化
- **补充说明**: 明确使用Mutex_Lock实现设备独占访问模式
- **设计模式**: 同一时刻只允许一个进程打开设备（类似单例访问模式）
- **影响范围**:
  - requirements.md 术语表（新增Mutex_Lock定义）
  - requirements.md 需求2（UART通信管理）
  - requirements.md 需求11（并发安全）
  - README.md（新增注意事项）

### 技术细节

#### GPIO中断配置要求
```c
// 引脚模式: 输入
// 中断触发: 双边沿（上升沿和下降沿）
// 中断处理: 记录触摸事件时间戳
// sysfs接口: 提供触摸事件查询
```

#### 设备树配置示例
```dts
fingerprint {
    compatible = "ds532-uart";
    status = "okay";
    uart = <&uart6>;
    touch-irq-gpios = <&gpio4 21 GPIO_ACTIVE_HIGH>;
    baudrate = <57600>;
};
```

### 待办事项
- [ ] 创建design.md设计文档
- [ ] 创建tasks.md任务列表
- [ ] 实现GPIO中断处理代码
- [ ] 实现设备独占访问逻辑
- [ ] 更新测试程序以验证中断功能

### 重要约束
⚠️ **目录隔离和文件独立性约束**：

#### 目录结构
- `.kiro/` - 只存放规范文档（requirements.md, design.md, tasks.md等）
- `kiro_source/` - 存放所有新代码和配置文件
- 顶层目录 - 保持现有文件不变

#### 文件约束
- `fingerprint_uart.c` 和 `fingerprint_test.c` 是独立文件，不能修改或参考
- 新实现使用独立的目录和文件名：
  - 目录：`kiro_source/`
  - 驱动：`kiro_source/ds532_driver.c` / `kiro_source/ds532_driver.h`
  - 测试：`kiro_source/ds532_test.c`
  - 设备树：`kiro_source/ds532_fingerprint.dts`
  - 编译：`kiro_source/Makefile`
- 所有代码从零开始编写，不依赖现有实现

### 文档状态
- ✅ requirements.md - 已更新
- ✅ design.md - 已创建
- ✅ tasks.md - 已创建（已更新目录结构）
- ✅ CHANGELOG.md - 本文件
- ✅ FILE_CONSTRAINTS.md - 已创建（已更新目录结构）
- ⏳ kiro_source/ - 待创建目录
- ⏳ kiro_source/ds532_driver.c - 待创建
- ⏳ kiro_source/ds532_driver.h - 待创建
- ⏳ kiro_source/ds532_test.c - 待创建
- ⏳ kiro_source/Makefile - 待创建
- ⏳ kiro_source/README.md - 待创建
