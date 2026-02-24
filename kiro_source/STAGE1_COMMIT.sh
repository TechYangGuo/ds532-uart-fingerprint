#!/bin/bash
# Stage 1 Git 提交脚本

echo "=========================================="
echo "Stage 1 Git 提交"
echo "=========================================="

# 添加文件
echo "添加文件到 Git..."
git add .kiro/specs/ds532-uart-driver/
git add kiro_source/*.c
git add kiro_source/*.h
git add kiro_source/*.dts
git add kiro_source/Makefile
git add kiro_source/*.md
git add kiro_source/*.sh
git add .gitignore

# 查看状态
echo ""
echo "当前状态："
git status

# 提交
echo ""
echo "创建提交..."
git commit -m "Stage 1: 完成最小可运行驱动框架

- Platform 驱动框架（设备树匹配、probe/remove）
- 字符设备接口（/dev/ds532_fp）
- 设备独占访问控制（原子变量+互斥锁）
- 基本文件操作（open/close/read/write/ioctl）
- 完整的规格文档（requirements/design/tasks）
- 测试指南和工作流文档
- 代码量：约 650 行
- 测试状态：7 项测试全部通过"

# 创建标签
echo ""
echo "创建标签..."
git tag -a v0.1-stage1 -m "Stage 1: 最小可运行驱动框架"

echo ""
echo "=========================================="
echo "提交完成！"
echo "=========================================="
echo ""
echo "查看提交："
git log --oneline -1
echo ""
echo "查看标签："
git tag
