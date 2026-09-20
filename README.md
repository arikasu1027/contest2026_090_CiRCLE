
# 合页

## 一、作品简介

合页是一套基于 openvela 与 LCKFB 黄山派（SF32LB52）开发板的智能灵感记录 AI 硬件原型。在 SF32LB52 GPU 驱动缺失、LVGL 严重卡顿的限制下，转向 VelaJS 快应用框架构建系统级 UI，实现锁屏、桌面 Launcher、设置、音乐播放器及基于 IMU 的"竹知了"传感器交互应用。同时完成 SF32LB52 音频驱动从零开发（521 行）、文件触发式跨进程 IPC 机制、蓝牙 PAN 框架搭建，并自建 vela_config_lookup Kconfig 智能查询 Skill。全程 AI-Native 开发，MiMoCode 为主力工具（113 session），单人 30 天完成驱动层到应用层的全栈交付。

**亮点：**
- VelaJS 快应用构建完整手表 Launcher，绕过 LVGL 无 GPU 加速的性能瓶颈
- SF32LB52 音频驱动全新开发，AUDCODEC DAC + PA42 Class-D PA 完整通路
- 自建 vela_config_lookup Skill，解决 AI 不了解 openvela Kconfig 依赖链的痛点
- 竹知了：加速度传感器触发蝉鸣音效，探索小屏设备物理交互新范式

## 二、选题方向

**AI 硬件产品创新 + 新硬件平台适配**

- AI 硬件产品创新：以 openvela + SF32LB52 构建小屏智能硬件原型，涵盖系统级 UI、音频、传感器交互等完整链路
- 新硬件平台适配：完成 SF32LB52 音频驱动全新开发、传感器接入、按键映射等硬件适配工作

## 三、目录结构


contest2026_090_CiRCLE/
├── app/
│   └── audio_player/          — C 音频守护进程（274行），轮询 /data/play_trigger，WAV 解析 + 正弦波测试
├── quickapp/
│   ├── maruhome/              — MaruHome Launcher 快应用（568行）
│   │   └── src/pages/
│   │       ├── lock/          — 锁屏页面（132行），上滑解锁
│   │       ├── home/          — 桌面页面（84行），3列应用磁贴网格
│   │       ├── settings/      — 设置页面（168行），蓝牙/显示/声音/系统/关于
│   │       └── music/         — 音乐页面（146行），播放控制
│   └── bambockd/              — 竹知了交互应用（162行），IMU 加速度触发蝉鸣
├── vendor/
│   └── sifli/boards/.../src/
│       └── sf32lb_audio.c     — SF32LB52 音频驱动（521行），AUDCODEC DAC + PA42
├── board/
│   └── contest_board/         — 开发板配置文件
├── logs/
│   └── arikasu1027/           — AI Coding 日志（122 个 session），由插件自动生成
└── docs/                      — 技术报告及相关文档


## 四、运行方式

### 环境准备

```bash
# 1. 克隆仓库
git clone https://github.com/open-vela/contest2026_090_CiRCLE.git
cd contest2026_090_CiRCLE

# 2. 拉取 openvela 主仓（如尚未配置）
# 参考 openvela 官方文档完成 repo init / repo sync
```

### 编译

```bash
# 使用 contest_board defconfig 编译（基于 SF32LB52）
./build.sh vendor/sifli/boards/sf32lb52/contest_board --cmake

# 首次编译或切换 defconfig 后建议：
# ./build.sh vendor/sifli/boards/sf32lb52/contest_board --cmake distclean
```

### 烧录

```bash
# 编译产物位于 \openvela\cmake_out\lckfb_huangshan_pi_nsh\ 目录，通过 SF32LB52 烧录工具写入开发板
# 具体烧录方式参考 LCKFB 黄山派官方文档
```

### 运行

```bash
# 烧录完成后开发板自动启动，通过 USB 转 UART 连接串口：
# 波特率 1000000，nsh 终端可用

# 启动后系统自动进入 MaruHome Launcher（锁屏 → 桌面）
# 桌面磁贴可点击进入设置、音乐等页面

# 手动触发音频播放器（Native 守护进程）：
nsh> audio_player &

# 竹知了应用：从桌面点击进入，摇晃开发板触发蝉鸣
# 这个修炸了用不了
```

### 自定义 Skill（vela_config_lookup）

```bash
# Skill 位于 .claude/skills/vela_config_lookup/
# 在 MiMoCode 中使用：
# "用 vela_config_lookup 查一下 CONFIG_FEATURE_SYSTEM_ 相关的配置"

# 或手动测试：
cd .claude/skills/vela_config_lookup
bash test_skill.sh
```

## 五、AI Coding 使用说明

### 工具链

| 工具 | 用途 | 使用规模 |
|---|---|---|
| MiMoCode（MiMo-v2.5-pro） | 主力开发工具，代码生成、调试、重构 | 113 session，2,689 事件 |
| OpenCode | 早期技术探索 | 4 session，1,585 事件 |
| velajs-mcp | VelaJS 快应用调试 | 27 个工具，3 分钟配置 |

### AI 协作流程

1. **需求拆解**：将"在 SF32LB52 上跑 openvela 系统级 UI"拆分为驱动适配、框架选型、应用开发、IPC 设计等子任务，逐个与 AI 对话推进
2. **方案设计**：LVGL → VelaJS 快应用的技术栈切换决策由 AI 分析 GPU 驱动现状后提出建议
3. **编码实现**：快应用页面（730 行）、音频播放器（298 行）、音频驱动（521 行）均由 AI 辅助生成，人工审查修正
4. **调试排错**：蓝牙 PAN 依赖链分析、cmake 构建冲突排查、defconfig 配置等均通过 AI 对话完成
5. **工具建设**：vela_config_lookup Skill 由 AI 辅助开发（295 行核心引擎），解决了 AI 自身不了解 Kconfig 依赖链的问题

### 实际帮助

- 30 天单人完成驱动层到应用层全栈交付，AI 承担了约 1,629 行自有代码的大部分生成工作
- vela_config_lookup Skill 避免了盲目修改 defconfig 导致的反复编译失败
- 蓝牙 PAN 多 PR 协同合入的依赖分析（#121、#41、vendor_sifli #31）由 AI 辅助完成

完整对话日志见 `logs/arikasu1027/` 目录（122 个 session，由 AI Coding 插件自动生成，未手动删改）。

