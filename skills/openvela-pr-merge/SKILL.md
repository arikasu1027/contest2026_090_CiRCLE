---
name: openvela-pr-merge
description: "分析并合入 openvela 上游 PR 到本地工作区。覆盖：PR 内容分析、依赖链追踪、cherry-pick/merge 执行、编译验证、冲突解决。触发场景：用户说合入 PR、cherry-pick、merge PR、分析某个 PR、查看 PR 改了什么。"
---

# openvela-pr-merge

分析并合入 openvela 上游 PR 的标准化工作流。

## 调用时机

- 用户说 "帮我合入这个 PR"
- 用户给出 PR 链接要求分析
- 用户说 "cherry-pick 这个提交"
- 用户说 "分析一下 #XX 和 #YY 的差异"

## 工作流

### Phase 1: PR 分析

```bash
# 获取 PR 信息
gh pr view <PR_NUMBER> --repo <OWNER/REPO> --json title,body,files,commits

# 查看 PR 改动的文件
gh pr diff <PR_NUMBER> --repo <OWNER/REPO>

# 分析依赖关系
# 检查 PR 描述中是否提到了依赖其他 PR
# 检查改动文件是否涉及 Kconfig/CMakeLists 变更
```

输出：PR 摘要（标题、改动文件列表、依赖的其他 PR、潜在风险）

### Phase 2: 依赖链追踪

```bash
# 检查本地已有的提交
git log --oneline -20

# 检查目标分支
git branch -a | grep <TARGET_BRANCH>

# 分析 PR 的 parent commit 是否在本地
git merge-base --is-ancestor <PR_COMMIT> HEAD
```

### Phase 3: 执行合入

**方式 A: cherry-pick（推荐，精确控制）**
```bash
git fetch <REMOTE> <BRANCH>
git cherry-pick <COMMIT_HASH>
# 如有冲突：解决后 git cherry-pick --continue
```

**方式 B: merge（保留完整历史）**
```bash
git fetch <REMOTE> <BRANCH>
git merge <COMMIT_HASH> --no-ff -m "Merge PR #XX: <title>"
```

### Phase 4: 编译验证

```bash
# 使用 openvela-build skill 的编译流程
./build.sh <BOARD_CONFIG> --cmake -j$(nproc)
```

### Phase 5: 冲突解决（如需要）

常见冲突类型及处理：
- **Kconfig 冲突**：对比上下游差异，保留新增配置项
- **CMakeLists 冲突**：检查新增的 source file 是否需要条件编译
- **头文件冲突**：检查 API 签名变化，适配本地代码

## 约束

- 合入前必须编译验证通过
- 不要直接修改生产仓库（nuttx/、apps/、packages/），走 PR 流程
- 记录每个合入的 PR 编号和合入方式

