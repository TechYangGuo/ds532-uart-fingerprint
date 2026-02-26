# DS532 UART驱动项目结构说明

## 📁 完整目录结构

```
fingerprint/                                    # 项目根目录
│
├── .kiro/                                      # Kiro规范文档目录
│   └── specs/
│       └── ds532-uart-driver/
│           ├── requirements.md                 ✅ 需求文档（12个需求）
│           ├── design.md                       ✅ 设计文档（完整技术设计）
│           ├── tasks.md                        ✅ 任务列表（14个主任务）
│           ├── CHANGELOG.md                    ✅ 变更记录
│           ├── FILE_CONSTRAINTS.md             ✅ 文件约束说明
│           └── PROJECT_STRUCTURE.md            ✅ 本文件
│
├── kiro_source/                                # 新代码目录（完全独立）
│   ├── README.md                               ✅ 项目说明
│   ├── ds532_driver.c                          ⏳ 待创建 - 驱动源文件
│   ├── ds532_driver.h                          ⏳ 待创建 - 驱动头文件
│   ├── ds532_test.c                            ⏳ 待创建 - 测试程序
│   ├── ds532_fingerprint.dts                   ⏳ 待创建 - 设备树配置
│   └── Makefile                                ⏳ 待创建 - 编译配置
│
├── fingerprint_uart.c                          ❌ 不可修改 - 现有驱动
├── fingerprint_test.c                          ❌ 不可修改 - 现有测试
├── fingerprint.dts                             ❌ 不可修改 - 现有设备树
├── Makefile                                    ❌ 不可修改 - 现有编译配置
└── README.md                                   ❌ 不可修改 - 现有说明文档
```

## 📋 目录功能说明

### 1. `.kiro/` - 规范文档目录

**用途**: 存放Kiro规范驱动开发的所有文档

**内容**:
- `requirements.md` - 需求文档
  - 12个核心需求
  - 每个需求包含用户故事和验收标准
  - 使用EARS模式编写
  
- `design.md` - 设计文档
  - 系统架构设计
  - 数据结构设计
  - DS532协议详解（基于官方手册）
  - 接口设计
  - 并发控制设计
  - 9个正确性属性
  - 完整的测试策略
  
- `tasks.md` - 任务列表
  - 14个主任务
  - 60+子任务
  - 每个任务都关联到具体需求
  - 包含可选的单元测试任务
  
- `CHANGELOG.md` - 变更记录
  - 记录所有重要修改
  - 包含修改原因和影响范围
  
- `FILE_CONSTRAINTS.md` - 文件约束说明
  - 详细说明目录隔离原则
  - 列出不可修改的文件
  - 说明新文件的创建位置
  
- `PROJECT_STRUCTURE.md` - 本文件
  - 项目结构总览
  - 快速开始指南

**特点**: 只包含文档，不包含代码

### 2. `kiro_source/` - 新代码目录

**用途**: 存放本项目的所有新代码和配置

**内容**:
- `ds532_driver.c` - 驱动源文件
  - Platform驱动框架
  - TTY/UART通信
  - DS532协议封装
  - GPIO中断处理
  - 字符设备接口
  - 并发控制
  - 错误处理
  
- `ds532_driver.h` - 驱动头文件
  - 数据结构定义
  - 协议常量定义
  - 函数声明
  
- `ds532_test.c` - 测试程序
  - 交互式菜单
  - 所有IOCTL命令测试
  - GPIO中断测试
  - 并发访问测试
  
- `ds532_fingerprint.dts` - 设备树配置
  - UART6配置
  - GPIO中断配置
  - 波特率等参数配置
  
- `Makefile` - 编译配置
  - 内核模块编译
  - 测试程序编译
  - 部署脚本
  
- `README.md` - 项目说明
  - 编译方法
  - 部署方法
  - 使用说明
  - 故障排查

**特点**: 完全独立，与顶层目录的现有文件完全隔离

### 3. 顶层目录 - 现有项目文件

**内容**: 现有项目的文件

**约束**: 所有现有文件保持不变，不做任何修改

**文件**:
- `fingerprint_uart.c` - 现有驱动
- `fingerprint_test.c` - 现有测试
- `fingerprint.dts` - 现有设备树
- `Makefile` - 现有编译配置
- `README.md` - 现有说明文档

## 🚀 快速开始

### 1. 查看规范文档

```bash
# 查看需求文档
cat .kiro/specs/ds532-uart-driver/requirements.md

# 查看设计文档
cat .kiro/specs/ds532-uart-driver/design.md

# 查看任务列表
cat .kiro/specs/ds532-uart-driver/tasks.md

# 查看文件约束
cat .kiro/specs/ds532-uart-driver/FILE_CONSTRAINTS.md
```

### 2. 开始实现

有两种方式：

#### 方式A: 手动实现

按照 `tasks.md` 中的任务顺序，逐个实现：

```bash
# 1. 进入代码目录
cd kiro_source

# 2. 创建驱动头文件
vim ds532_driver.h

# 3. 创建驱动源文件
vim ds532_driver.c

# 4. 创建测试程序
vim ds532_test.c

# 5. 创建Makefile
vim Makefile

# 6. 编译
make
```

#### 方式B: 自动执行任务

告诉Kiro执行所有任务：

```
run all tasks
```

或者执行特定任务：

```
execute task 1.1
```

### 3. 编译和测试

```bash
cd kiro_source
make
make install
```

## 📝 开发流程

### 阶段1: 规范文档（已完成）

- ✅ 需求分析
- ✅ 设计文档
- ✅ 任务分解

### 阶段2: 驱动实现（进行中）

1. 搭建驱动框架
2. 实现DS532协议封装
3. 实现TTY/UART通信
4. 实现GPIO中断处理
5. 实现并发控制
6. 实现字符设备接口
7. 实现错误处理

### 阶段3: 测试程序（待开始）

1. 创建测试程序框架
2. 实现基本测试功能
3. 实现GPIO中断测试
4. 实现并发访问测试

### 阶段4: 集成测试（待开始）

1. 完整指纹录入流程测试
2. 完整指纹匹配流程测试
3. GPIO中断响应测试
4. 性能测试
5. 稳定性测试

### 阶段5: 文档和交付（待开始）

1. 代码规范检查
2. 添加代码注释
3. 编写使用文档
4. 编写故障排查指南
5. 最终验证

## 🎯 关键约束

### 目录隔离

- ✅ 规范文档放在 `.kiro/specs/ds532-uart-driver/`
- ✅ 新代码放在 `kiro_source/`
- ❌ 不修改顶层目录的任何现有文件

### 文件独立性

- ❌ 不修改 `fingerprint_uart.c`
- ❌ 不修改 `fingerprint_test.c`
- ❌ 不参考上述两个文件的代码
- ✅ 所有代码从零开始编写

### 命名规范

- 驱动文件: `ds532_driver.c/h`
- 测试程序: `ds532_test.c`
- 函数前缀: `ds532_`
- 结构体前缀: `struct ds532_`
- 宏定义前缀: `DS532_`

## 📊 项目状态

### 已完成 ✅

- [x] 需求文档（12个需求）
- [x] 设计文档（完整技术设计）
- [x] 任务列表（14个主任务）
- [x] 变更记录
- [x] 文件约束说明
- [x] 项目结构说明
- [x] kiro_source目录创建
- [x] kiro_source/README.md

### 进行中 ⏳

- [ ] 驱动实现
- [ ] 测试程序实现

### 待开始 📋

- [ ] 集成测试
- [ ] 文档完善
- [ ] 最终验证

## 🔗 相关链接

### 规范文档

- [需求文档](requirements.md)
- [设计文档](design.md)
- [任务列表](tasks.md)
- [文件约束](FILE_CONSTRAINTS.md)

### 参考资料

- DS532用户手册: `../../source/方形指纹DS532用户使用手册/`
- Linux内核文档: `/home/book/Workspace/wds/sdk/imx6ull/100ask_imx6ull-sdk/Linux-4.9.88/Documentation/`

## 📞 支持

如有问题，请查看：
1. `FILE_CONSTRAINTS.md` - 文件约束说明
2. `kiro_source/README.md` - 项目使用说明
3. `design.md` - 技术设计文档

---

**最后更新**: 2026-02-23  
**状态**: 规范文档已完成，准备开始实现
