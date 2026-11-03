# 合页 · CiRCLE

基于 **openvela + LCKFB 黄山派（SF32LB52）** 的小屏智能硬件作品：OLED 触摸屏 + Vela 快应用系统 UI，底层服务用 C 保证效率。

> 灵感来自 Cybiko / 任天堂 NDS「方舟雷达」一类近场社交玩具；原设想为电动自行车 AR 导航，因硬件受阻改为可携带的「合页」形态。

---

## 一、项目简介

| 项 | 说明 |
|----|------|
| 硬件 | 黄山派 SF32LB52，OLED 触摸屏，体积小、便于做外壳 |
| 系统 | openvela（NuttX + 快应用运行时） |
| UI | **VelaJS 快应用**（曾试原生 LVGL，美观/完成度不够后整体转向） |
| 底层 | C 原生服务 / 驱动（音频、蓝牙、时间守护等） |
| 形态 | 「合页」——曾尝试类似 MIX Flip 的铰链，后简化结构 |

「擦肩消息」：两台设备通过 BLE 广播昵称与联系方式，靠近时互相发现；可用 DeviceID 错开广播相位。

---

## 二、当前软件能力（以本仓代码为准）

单应用 **MaruHome**（`com.circle.home`）多页面切换，入口 `pages/lock`：

| 页面 | 路径 | 说明 |
|------|------|------|
| 锁屏 | `pages/lock` | 时钟，上滑解锁 |
| 桌面 | `pages/home` | 应用磁贴网格 |
| 计算器 | `pages/calculator` | 本地四则运算 |
| 设置 | `pages/settings` | 名片展示、近场交换开关、时间、亮度 |
| 竹知了 | `pages/cicada` | 加速度阈值触发蝉鸣（`system.sensor` + `system.audio`） |
| 音乐 | `pages/music` | 本地文件扫描 / 播放（依赖 media 栈，见「已知限制」） |
| 消息 | `pages/messages` | 近场名片占位（BLE 交换待固件 feature 就绪） |
| 对讲 | `pages/intercom` | 占位页 |

构建产物：`quickapp/maruhome/dist/com.circle.home.debug.1.0.0.rpk`

---

## 三、目录结构（本仓 `dev-ai-contest-2026`）

```
contest2026_090_CiRCLE/
├── app/
│   ├── btpand/                 # 蓝牙 PAN 相关原生程序
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   └── btpand_main.c
│   └── timesetd/               # 时间设置守护：轮询 /data/tmp/settime.req → settimeofday
│       ├── CMakeLists.txt
│       ├── Kconfig             # CONFIG_LVX_USE_DEMO_CONTEST2026_090_TIMESETD
│       └── timesetd_main.c
├── board/
│   └── hyw/configs/hyw/
│       └── defconfig           # 本板实际使用的 defconfig
├── contest2026_090_CiRCLE.xml # repo manifest（linkfile 等）
└── quickapp/
    └── maruhome/               # 系统桌面快应用（单应用多页面）
        ├── package.json
        ├── dist/*.rpk
        └── src/
            ├── manifest.json
            ├── app.ux
            ├── common/         # 图标、cicada.wav 等资源
            └── pages/          # lock / home / calculator / settings /
                                # cicada / music / messages / intercom
```

说明：历史目录 `quickapp/bambockd`、`app/audio_player` 已在本分支删除，功能并入或由新方案替代。

---

## 四、构建与运行

### 固件（openvela 树内）

实际构建配置（见 `cmake_out` 中 `BOARD_CONFIG`）：

```text
contest2026_090_CiRCLE/board/hyw/configs/hyw
```

按你们现有 hyw 流程 configure + 编译 + 烧录。  
串口 nsh 后：

```sh
# 时间守护（需 defconfig 打开对应 LVX_USE_DEMO 开关并编入）
# 请求文件：/data/tmp/settime.req  内容为 unix 秒或 "YYYY-MM-DD HH:MM:SS"
# 应答：    /data/tmp/settime.ack

# 蓝牙 PAN 相关原生程序（以 btpand 用法为准）
```

### 快应用

```bash
cd quickapp/maruhome
npm install
npm run build
# 产物 dist/com.circle.home.debug.1.0.0.rpk
```

安装到设备后：锁屏上滑 → 桌面 → 各功能页左缘侧滑返回。

---

## 五、上游贡献（PR）

本项目向 openvela 相关仓库提交的定制修改：

| 仓库 | PR | 内容摘要 |
|------|-----|----------|
| [open-vela/frameworks_bluetooth](https://github.com/open-vela/frameworks_bluetooth/pull/595) | [#595](https://github.com/open-vela/frameworks_bluetooth/pull/595) | **PANU / BNEP 网络数据面**：`sal_pan_interface.c` 实现 BNEP RX（压缩帧 0x02/0x04 与完整以太网帧），修复 COMPRESSED_SOURCE 分发，补充诊断日志 |
| [open-vela/vendor_sifli](https://github.com/open-vela/vendor_sifli/pull/39) | [#39](https://github.com/open-vela/vendor_sifli/pull/39) | **SF32LB52 蓝牙适配**：`sf32lb52_bt_adapter.c` 对接 Zblue、HCI 路由与 Sniff 功耗钩子；`sf32lb52_bth4.c` H4 UART ACL 解析；堆分配相关调整 |
| [open-vela/external_zblue](https://github.com/open-vela/external_zblue/pull/235) | [#235](https://github.com/open-vela/external_zblue/pull/235) | **Zblue PAN/BNEP 适配**：`l2cap_br.c` 修复 BNEP（PSM 0x000F）连接与 MTU；`hci_core.c` 修复 HCI 与 Sniff/功耗交互，避免 PAN 通路被打断 |

对应 PPT 中「蓝牙链接 / PAN」等技术工作。

---

## 六、已知限制（诚实清单）

| 项 | 状态 |
|----|------|
| 快应用跨应用启动（jumpApp） | 固件未提供 feature，桌面仅页内路由 |
| 近场名片完整 BLE 交换 | 协议栈侧有 PAN/BNEP PR；快应用 `system.bluetooth.ble` 视固件而定，UI 可先占位 |
| 本地音乐播放 | 依赖 media server + FFmpeg 配置，未配齐前播放不可用 |
| 设置页时间 | 通过 `timesetd` + `/data/tmp/settime.req` 文件 IPC，需固件编入守护进程 |
| 加速度 → 快应用 | uORB 可读；`system.sensor` 订阅在部分固件上仍可能失败 |
| 输入法 | 第三方输入法组件因体积/内存未采用，名片字段为只读展示 |

---

## 七、硬件杂谈（答辩轶事，摘自 PPT）

- 扬声器：K80 至尊版大师级同轴  
- 电池：Xiaomi Watch S4 Sport 高密度电芯  
- 震动马达：小米 14 Pro  
- 曾构思索尼激光投影光机 + HUD 夜间路面投影，因硬件受阻放弃  

---

## License

以各子仓库及 openvela 上游 License 为准；本仓代码遵循仓库内声明。
