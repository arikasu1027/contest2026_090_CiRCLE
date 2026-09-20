## Skill: vela_config_lookup

当用户询问 OpenVela 的 Kconfig 配置相关问题时，使用此 skill。

### 调用时机
- 用户问 "XXX 配置怎么开"
- 用户问 "CONFIG_XXX 依赖什么"
- 用户需要编写/修改 defconfig
- 用户问 "有没有 XXX 相关的配置项"

### 调用方式

    python3 vela_config_lookup/dispatch.py "用户的问题" /path/to/openvela

### 结果解读
- status=found -> 配置已启用，展示 value 和依赖
- status=not_found -> 配置存在但未启用，展示 defconfig_snippet
- status=fuzzy_match -> 展示 matches 列表让用户选择

### 约束
- 不要将 Kconfig 源文件内容复制到上下文中
- 只传递脚本返回的精简 JSON
- 批量查询优于逐个对话
