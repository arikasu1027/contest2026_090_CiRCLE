# vela_config_lookup

OpenVela Kconfig 配置项智能查询 Skill（v0.1.0）。

> 已转换为 SKILL.md 格式，可被 MiMoCode skill 系统自动加载。

## 文件

| 文件 | 作用 |
|------|------|
| **SKILL.md** | 技能注册与使用说明（MiMoCode 标准格式） |
| skill.yaml | 旧版注册配置（保留兼容） |
| vela_config_query.py | 核心查询引擎（295行） |
| dispatch.py | 子代理调度器 |
| agent_prompt.md | Agent 提示词模板 |
| test_skill.sh | 自测脚本(内置 mock 数据) |

## 快速使用

    # 自测
    bash vela_config_lookup/test_skill.sh

    # 精确查询
    python3 vela_config_lookup/vela_config_query.py -q CONFIG_NET_TCP -w /opt/openvela

    # 模糊搜索
    python3 vela_config_lookup/vela_config_query.py -q bluetooth -w /opt/openvela

    # 子代理调度
    python3 vela_config_lookup/dispatch.py "CONFIG_NET 依赖什么" /opt/openvela

## 工作流

    用户: "开启蓝牙网络"
      -> Agent 调用 dispatch.py
      -> vela_config_query.py 读 .config + 扫描 Kconfig
      -> 返回精简 JSON (含 defconfig_snippet)
      -> Agent 写入 defconfig
      -> CMake 重新构建
