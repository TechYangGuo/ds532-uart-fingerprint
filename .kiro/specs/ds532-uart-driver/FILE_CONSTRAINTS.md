# 文件约束说明

## ⚠️ 重要：文件独立性约束

### 不可修改的现有文件

以下文件是独立的，**绝对不能修改或参考**：

1. ❌ `fingerprint_uart.c`
   - 现有的驱动实现
   - 保持独立，不做任何修改
   - 新驱动不能参考此文件的代码

2. ❌ `fingerprint_test.c`
   - 现有的测试程序
   - 保持独立，不做任何修改
   - 新测试程序不能参考此文件的代码

### 新创建的文件

本项目将创建以下全新的文件：

#### 驱动文件
1. ✅ `ds532_driver.c`
   - 新的驱动源文件
   - 从零开始实现
   - 基于 requirements.md 和 design.md 规范

2. ✅ `ds532_driver.h`
   - 新的驱动头文件
   - 定义数据结构和接口
   - 定义DS532协议常量

#### 测试文件
3. ✅ `ds532_test.c`
   - 新的用户空间测试程序
   - 从零开始实现
   - 基于 requirements.md 中的测试需求

#### 配置文件
4. ✅ `fingerprint.dts`
   - 设备树配置（已更新）
   - 可以修改和完善

5. ✅ `Makefile`
   - 编译配置（需要更新）
   - 添加新文件的编译规则

## 实现原则

### 1. 完全独立实现
- 所有新代码放在 `kiro_source/` 目录下
- 不修改顶层目录的任何现有文件
- 所有代码从零开始编写
- 不复制、不参考现有的 `fingerprint_uart.c` 和 `fingerprint_test.c`
- 严格按照规范文档实现

### 2. 目录隔离
- **规范文档**: 存放在 `.kiro/specs/ds532-uart-driver/`
- **新代码**: 存放在 `kiro_source/`
- **现有代码**: 保持在顶层目录，不做任何修改

### 3. 命名规范
- 驱动文件使用 `ds532_` 前缀
- 函数命名：`ds532_xxx()`
- 结构体命名：`struct ds532_xxx`
- 宏定义：`DS532_XXX`

### 4. 设备节点
- 新驱动创建的设备节点：`/dev/ds532_fp`
- 与现有的设备节点不冲突

### 5. 模块名称
- 新驱动模块名：`ds532_driver.ko`
- 与现有模块不冲突

## 文件清单

### 项目目录结构

```
fingerprint/
├── .kiro/                          # Kiro规范文档目录（只存放规范）
│   └── specs/
│       └── ds532-uart-driver/
│           ├── requirements.md      ✅ 需求文档
│           ├── design.md           ✅ 设计文档
│           ├── tasks.md            ✅ 任务列表
│           ├── CHANGELOG.md        ✅ 变更记录
│           └── FILE_CONSTRAINTS.md ✅ 本文件
│
├── kiro_source/                    # 新代码目录（完全独立）
│   ├── ds532_driver.c              ⏳ 待创建（新驱动源文件）
│   ├── ds532_driver.h              ⏳ 待创建（新驱动头文件）
│   ├── ds532_test.c                ⏳ 待创建（新测试程序）
│   ├── ds532_fingerprint.dts       ⏳ 待创建（新设备树配置）
│   ├── Makefile                    ⏳ 待创建（新编译配置）
│   └── README.md                   ⏳ 待创建（新项目说明）
│
├── fingerprint_uart.c              ❌ 不可修改（现有驱动）
├── fingerprint_test.c              ❌ 不可修改（现有测试）
├── fingerprint.dts                 ❌ 不可修改（现有设备树）
├── Makefile                        ❌ 不可修改（现有编译配置）
└── README.md                       ❌ 不可修改（现有说明文档）
```

### 目录说明

#### `.kiro/` 目录
- **用途**: 存放Kiro规范文档
- **内容**: requirements.md, design.md, tasks.md等规范文件
- **特点**: 只包含文档，不包含代码

#### `kiro_source/` 目录
- **用途**: 存放本项目的所有新代码和配置
- **内容**: 驱动源码、测试程序、设备树、Makefile等
- **特点**: 完全独立，与顶层目录的现有文件完全隔离

#### 顶层目录
- **内容**: 现有项目的文件（fingerprint_uart.c等）
- **约束**: 所有现有文件保持不变，不做任何修改

## 编译和部署

### 编译方式

在 `kiro_source/` 目录下编译：

```bash
cd kiro_source
make
```

### Makefile 配置

`kiro_source/Makefile` 内容：

```makefile
# DS532指纹模块UART驱动 Makefile
# 平台: IMX6ULL / Linux 4.9.88

obj-m += ds532_driver.o

KDIR := /home/book/Workspace/wds/sdk/imx6ull/100ask_imx6ull-sdk/Linux-4.9.88

ARCH_ARGS := CROSS_COMPILE=/home/book/Workspace/wds/sdk/imx6ull/100ask_imx6ull-sdk/Buildroot_2020.02.x/output/host/bin/arm-buildroot-linux-gnueabihf- ARCH=arm
CC := /home/book/Workspace/wds/sdk/imx6ull/100ask_imx6ull-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/arm-buildroot-linux-gnueabihf-gcc

PWD := $(shell pwd)

EXTRA_CFLAGS += -DDEBUG

all: 
	make $(ARCH_ARGS) -C $(KDIR) M=$(PWD) modules 
	$(CC) ds532_test.c -o ds532_test

install:
	adb push *.ko ds532_test /root/ds532/

clean: 
	make $(ARCH_ARGS) -C $(KDIR) M=$(PWD) modules clean 
	rm -f ds532_test

.PHONY: all install clean
```

### 部署说明

#### 两套独立系统

系统中将存在两套独立的驱动和测试程序：

**旧系统（顶层目录，保持不变）**
- 驱动：`fingerprint_uart.ko`
- 测试：`fingerprint_test`
- 位置：顶层目录

**新系统（kiro_source目录，本项目）**
- 驱动：`ds532_driver.ko`
- 测试：`ds532_test`
- 设备：`/dev/ds532_fp`
- 位置：`kiro_source/` 目录

两套系统互不干扰，可以独立加载和测试。

## 验证清单

在开始实现前，请确认：

- [ ] 理解了目录隔离原则
- [ ] 所有新代码将放在 `kiro_source/` 目录
- [ ] 不会修改顶层目录的任何现有文件
- [ ] 不会修改 `fingerprint_uart.c`
- [ ] 不会修改 `fingerprint_test.c`
- [ ] 不会参考上述两个文件的代码
- [ ] 将在 `kiro_source/` 目录创建新的 `ds532_driver.c/h` 文件
- [ ] 将在 `kiro_source/` 目录创建新的 `ds532_test.c` 文件
- [ ] 所有实现基于规范文档（requirements.md 和 design.md）

## 工作流程

### 1. 创建目录
```bash
mkdir -p kiro_source
cd kiro_source
```

### 2. 创建文件
按照 tasks.md 中的任务顺序创建文件：
- `ds532_driver.h` - 驱动头文件
- `ds532_driver.c` - 驱动源文件
- `ds532_test.c` - 测试程序
- `ds532_fingerprint.dts` - 设备树配置
- `Makefile` - 编译配置
- `README.md` - 项目说明

### 3. 编译测试
```bash
cd kiro_source
make
```

### 4. 部署
```bash
cd kiro_source
make install
```

## 注意事项

1. **代码审查**：在提交代码前，确保没有从现有文件复制任何代码
2. **命名冲突**：使用 `ds532_` 前缀避免与现有代码冲突
3. **设备节点**：新驱动使用 `/dev/ds532_fp`，不与现有设备冲突
4. **独立测试**：新旧系统可以独立测试，互不影响

---

**最后更新**: 2026-02-23
**状态**: 规范文档已完成，准备开始实现
