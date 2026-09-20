---
name: vela_config_lookup
description: "查询 OpenVela 项目的 Kconfig 配置项。支持：精确查找 .config 中的值、依赖分析（depends on/selects/implies）、模糊搜索未启用项、生成 defconfig 片段。触发场景：用户问 CONFIG_XXX 怎么开、配置依赖什么、defconfig 怎么写、有没有相关配置项。"
---

# vela_config_lookup

OpenVela Kconfig 配置项智能查询工具。

## 调用时机

- 用户问 "XXX 配置怎么开"
- 用户问 "CONFIG_XXX 依赖什么"
- 用户需要编写/修改 defconfig
- 用户问 "有没有 XXX 相关的配置项"

## 使用方式

```bash
# 精确查询
python3 scripts/vela_config_query.py -q CONFIG_NET_TCP -w /path/to/openvela

# 模糊搜索
python3 scripts/vela_config_query.py -q bluetooth -w /path/to/openvela

# 自然语言调度（从问题中提取 CONFIG 名称）
python3 scripts/dispatch.py "CONFIG_NET 依赖什么" /path/to/openvela
```

## 参数

| 参数 | 说明 |
|------|------|
| `-q, --query` | 配置项名称（精确或模糊），如 `CONFIG_NET_TCP` 或 `bluetooth` |
| `-w, --workspace` | OpenVela 工作区根目录 |
| `-b, --build-dir` | 构建输出目录（含 .config），默认 `workspace/build` |
| `-f, --fuzzy` | 启用模糊搜索 |
| `-n, --max-results` | 模糊搜索最大结果数，默认 20 |

## 结果解读

| status | 含义 | 操作 |
|--------|------|------|
| `found` | 配置已存在于 .config | 展示 value 和依赖关系 |
| `not_found` | 配置在 Kconfig 中定义但未启用 | 展示 `defconfig_snippet`（可直接粘贴） |
| `fuzzy_match` | 未精确匹配，返回候选列表 | 让用户选择正确的配置项 |

## 输出格式

```json
{
  "status": "found|not_found|fuzzy_match",
  "config": "CONFIG_XXX",
  "value": "y",
  "enabled": true,
  "depends_on": ["CONFIG_A", "CONFIG_B"],
  "selects": ["CONFIG_C"],
  "defconfig_snippet": "# depends on:\n# CONFIG_A=y\nCONFIG_XXX=y"
}
```

## 约束

- 不要将 Kconfig 源文件内容复制到上下文中，只传递脚本返回的精简 JSON
- 批量查询优于逐个对话
- 修改 defconfig 后需重新编译验证

## 自测

```bash
bash scripts/test_skill.sh
```
