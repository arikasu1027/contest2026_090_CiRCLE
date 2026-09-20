---
name: vela-config-recommend
description: "根据功能目标自动推荐完整的 openvela defconfig 配置集。输入功能描述（如'启用蓝牙PAN上网'），输出所有需要开启的 CONFIG_ 项及依赖链。触发场景：用户说'我要做XXX功能，需要开什么配置'、'帮我配一个完整的XXX defconfig'、'启用XXX需要哪些配置'。"
---

# vela-config-recommend

根据功能目标推荐完整的 defconfig 配置集。

> 与 `vela_config_lookup`（查询单个配置）互补，本 skill 解决"给定目标，推荐完整配置"的问题。

## 调用时机

- 用户说 "我要做蓝牙上网，需要开什么配置"
- 用户说 "帮我配一个能跑音频的 defconfig"
- 用户说 "启用 XXX 功能需要哪些配置"

## 工作流

### Step 1: 功能分析

将用户的功能描述映射到 openvela 的配置域：

| 功能域 | 关键配置前缀 | 示例 |
|--------|-------------|------|
| 蓝牙 | `CONFIG_BT_*`, `CONFIG_BLUETOOTH_*` | PAN、GATT、A2DP |
| 网络 | `CONFIG_NET_*`, `CONFIG_NET_TCP` | TCP/IP、DNS、TUN |
| 音频 | `CONFIG_AUDIO_*`, `CONFIG_DRIVERS_AUDIO` | PCM、CODEC、PA |
| 传感器 | `CONFIG_SENSORS_*`, `CONFIG_UORB` | 加速度、磁力、光感 |
| 显示 | `CONFIG_LCD_*`, `CONFIG_VIDEO_*`, `CONFIG_LV_*` | FB、LVGL、触摸 |
| 存储 | `CONFIG_MMCSD_*`, `CONFIG_FS_*` | SD卡、FAT、ROMFS |

### Step 2: 依赖链分析

使用 `vela_config_query.py` 递归解析依赖：

```bash
python3 scripts/vela_config_query.py -q <CONFIG> -w <WORKSPACE> -f
```

对每个依赖项递归查询，构建完整依赖树。

### Step 3: 冲突检测

检查已启用配置之间的潜在冲突：
- 同一外设的多个驱动（如 SPI SD vs MMC SD）
- 互斥的蓝牙栈（openvela framework vs NuttX native）
- 内存占用预估（Flash/RAM）

### Step 4: 生成 defconfig 片段

输出格式：
```
# === 功能: 蓝牙 PAN 上网 ===
# 依赖链: NET → BT → BLUETOOTH → BLUETOOTH_PAN
CONFIG_NET=y
CONFIG_NET_TCP=y
CONFIG_NET_UDP=y
CONFIG_BT=y
CONFIG_BT_CLASSIC=y
CONFIG_BLUETOOTH=y
CONFIG_BLUETOOTH_PAN=y
CONFIG_NET_TUN=y
CONFIG_NET_TUN_PKTSIZE=1518
# === 冲突检测: 无 ===
```

## 约束

- 生成的配置片段需用户确认后再写入 defconfig
- 每次修改后需 `make olddefconfig` 解析依赖
- 不要自动修改 defconfig，只推荐

## 实际案例（来自日志）

蓝牙 PAN 上网的完整配置链：
```
CONFIG_NET=y → CONFIG_NET_TCP=y → CONFIG_NET_UDP=y
CONFIG_BT=y → CONFIG_BT_CLASSIC=y → CONFIG_BT_H4=y
CONFIG_BLUETOOTH=y → CONFIG_BLUETOOTH_PAN=y
CONFIG_NET_TUN=y → CONFIG_NET_TUN_PKTSIZE=1518
```
