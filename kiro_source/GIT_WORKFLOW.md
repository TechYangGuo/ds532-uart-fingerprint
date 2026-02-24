# Git 工作流指南

## 📦 版本控制范围

本项目只对 **Kiro 生成的内容** 进行版本控制：

### ✅ 纳入版本控制的内容
```
.kiro/specs/ds532-uart-driver/    # 规格文档
  ├── requirements.md
  ├── design.md
  ├── tasks.md
  ├── CHANGELOG.md
  ├── FILE_CONSTRAINTS.md
  └── PROJECT_STRUCTURE.md

kiro_source/                       # 驱动源码和文档
  ├── ds532_driver.c
  ├── ds532_driver.h
  ├── ds532_fingerprint.dts
  ├── Makefile
  ├── README.md
  ├── STAGE1_SUMMARY.md
  ├── STAGE1_TEST_GUIDE.md
  ├── GIT_WORKFLOW.md
  └── user/                        # 未来的测试程序
      └── ds532_test.c

.gitignore                         # Git 忽略规则
```

### ❌ 不纳入版本控制的内容
- 顶层目录的原有文件（`fingerprint_uart.c`, `fingerprint_test.c` 等）
- `source/` 目录（厂商资料）
- 编译产物（`.o`, `.ko`, `.mod.c` 等）
- IDE 配置文件（`.vs/`, `.vscode/`）
- 临时文件和日志

---

## 🚀 Git 初始化步骤

### 1. 初始化 Git 仓库

```bash
cd /share/project1/smart_face/fingerprint
git init
```

### 2. 配置 Git 用户信息（如果还没配置）

```bash
git config user.name "你的名字"
git config user.email "你的邮箱"
```

### 3. 添加 Kiro 生成的文件

```bash
# 添加规格文档
git add .kiro/specs/ds532-uart-driver/

# 添加驱动源码
git add kiro_source/*.c
git add kiro_source/*.h
git add kiro_source/Makefile
git add kiro_source/*.dts
git add kiro_source/*.md

# 添加 .gitignore
git add .gitignore
```

### 4. 创建初始提交

```bash
git commit -m "Initial commit: DS532 UART driver Stage 1

- 完成 Platform 驱动框架
- 实现字符设备接口
- 实现设备独占访问控制
- 添加规格文档（requirements, design, tasks）
- 代码量：约 650 行"
```

---

## 📝 日常工作流

### 阶段性提交（推荐）

每完成一个阶段后提交：

```bash
# 查看修改
git status
git diff

# 添加修改的文件
git add kiro_source/

# 提交
git commit -m "Stage 2: 实现 DS532 协议封装

- 实现校验和计算函数
- 实现数据包构造函数
- 实现 6 个命令封装函数
- 实现响应包验证和解析"
```

### 功能性提交

完成某个具体功能后提交：

```bash
git add kiro_source/ds532_driver.c
git commit -m "实现 TTY/UART 通信模块

- 添加 TTY 设备打开和配置
- 实现数据包发送函数（500ms 超时）
- 实现数据包接收函数（2000ms 超时）"
```

### 文档更新提交

```bash
git add .kiro/specs/ds532-uart-driver/tasks.md
git commit -m "更新任务列表：标记 Stage 2 任务为已完成"
```

---

## 🔍 查看历史

### 查看提交历史

```bash
# 简洁格式
git log --oneline

# 详细格式
git log

# 图形化显示
git log --graph --oneline --all
```

### 查看某次提交的详细内容

```bash
git show <commit-hash>
```

### 查看文件修改历史

```bash
git log -p kiro_source/ds532_driver.c
```

---

## 🔄 分支管理（可选）

如果想尝试新功能而不影响主线：

```bash
# 创建并切换到新分支
git checkout -b feature/gpio-interrupt

# 开发和提交
git add kiro_source/
git commit -m "实现 GPIO 中断处理"

# 切换回主分支
git checkout main

# 合并分支
git merge feature/gpio-interrupt

# 删除分支
git branch -d feature/gpio-interrupt
```

---

## 📊 推荐的提交节点

### Stage 1: 最小可运行驱动框架 ✅
```bash
git commit -m "Stage 1: 完成最小可运行驱动框架"
```

### Stage 2: DS532 协议封装
```bash
git commit -m "Stage 2: 完成 DS532 协议封装"
```

### Stage 3: UART 通信
```bash
git commit -m "Stage 3: 完成 UART 通信模块"
```

### Stage 4: GPIO 中断
```bash
git commit -m "Stage 4: 完成 GPIO 中断处理"
```

### Stage 5: 测试和完善
```bash
git commit -m "Stage 5: 完成测试程序和文档"
```

---

## 🛠️ 常用命令速查

```bash
# 查看状态
git status

# 查看差异
git diff                    # 工作区 vs 暂存区
git diff --staged           # 暂存区 vs 仓库

# 添加文件
git add <file>              # 添加指定文件
git add .                   # 添加所有修改（小心使用）

# 提交
git commit -m "message"     # 提交并附带消息
git commit -am "message"    # 添加并提交已跟踪文件

# 撤销操作
git checkout -- <file>      # 撤销工作区修改
git reset HEAD <file>       # 取消暂存
git reset --soft HEAD^      # 撤销上次提交（保留修改）

# 查看历史
git log                     # 查看提交历史
git log --oneline           # 简洁格式
git reflog                  # 查看所有操作记录
```

---

## 💡 最佳实践

1. **频繁提交**：每完成一个小功能就提交
2. **清晰的提交信息**：说明做了什么，为什么做
3. **阶段性标记**：使用 tag 标记重要节点
4. **保持干净**：不提交编译产物和临时文件
5. **定期查看**：使用 `git status` 检查状态

---

## 🏷️ 标记重要版本

```bash
# 标记 Stage 1 完成
git tag -a v0.1-stage1 -m "Stage 1: 最小可运行驱动框架"

# 标记 Stage 2 完成
git tag -a v0.2-stage2 -m "Stage 2: DS532 协议封装"

# 查看所有标签
git tag

# 查看标签详情
git show v0.1-stage1
```

---

## 📌 注意事项

1. **不要提交编译产物**：`.gitignore` 已配置好
2. **不要提交原有文件**：只管理 Kiro 生成的内容
3. **提交前检查**：使用 `git status` 和 `git diff` 确认
4. **写好提交信息**：方便以后查找和理解

---

**当前状态**：Stage 1 已完成，可以创建初始提交 ✅
