# Claude Code 实验项目

## 项目概述
初次体验 Claude Code 的实验性项目，探索 Claude Code 的能力边界。

## 技术栈
- verilog

## 项目结构
按功能模块组织模块和包。

## 常用命令
- 构建/安装依赖：`pip install -e .` 或 `pip install -r requirements.txt`
- 运行：`python -m <module>`

## 代码风格
- 遵循 PEP 8 标准
- 使用标准 Python 命名约定（snake_case 变量/函数、PascalCase 类）
- 保持简洁，避免不必要的注释

## Git 权限
- 本地命令可直接执行（add、status、diff、log 等）
- **需要确认的操作**：
  - git commit（每次提交前）
  - 影响 GitHub 公开页面的操作（push、新分支首次推送、gh pr create 等）
  - **新建分支前通知用户**（含分支名和用途）
- 同样适用于 gh 相关命令
- **提交记录要求**：保持提交历史干净，尽可能少 commit

## 沟通约定
- 用户使用语音输入，可能存在错别字或谐音（如"渐新"= 建新、"cloud点MD"= CLAUDE.md、"shan了"= 删了）。遇到不确定的指令先确认再执行

## 注意事项
- 不要擅自修改配置文件，修改前先询问
