---
name: openvela-flash-test
description: "openvela 固件的编译-烧录-测试自动化循环。支持 CLI 模式和 IDE 模式，自动处理 distclean、增量编译、烧录验证。触发场景：用户说'编译试试'、'烧录测试'、'build and flash'、'编译完了怎么测'、'nsh 测试'。"
---

# openvela-flash-test

编译-烧录-测试自动化循环。

## 调用时机

- 用户说 "编译试试" / "烧录测试"
- 用户说 "build and flash"
- 用户说 "编译完了怎么测"
- 用户需要在开发板上验证固件

## 工作流

### Phase 1: 编译

```bash
cd /path/to/openvela

# 增量编译（推荐，快速）
./build.sh <BOARD_CONFIG> --cmake -j$(nproc)

# 全量重编（配置变更后必须）
./build.sh <BOARD_CONFIG> --cmake distclean
./build.sh <BOARD_CONFIG> --cmake -j$(nproc)
```

**编译产物检查**：
```bash
# 检查编译是否成功
ls -la cmake_out/<CONFIG>/nuttx
# 检查 Flash/RAM 占用
arm-none-eabi-size cmake_out/<CONFIG>/nuttx
```

### Phase 2: 烧录

**方式 A: esptool（ESP32 系列）**
```bash
esptool.py -c <chip> -p <port> write-flash 0x0 nuttx.bin
```

**方式 B: make flash（NuttX 标准）**
```bash
make flash ESPTOOL_PORT=<port> ESPTOOL_BINDIR=./
```

**方式 C: VelaJS MCP（快应用）**
```bash
# 通过 velajs-mcp 的 start_debug_cli 工具
# 自动处理构建+推送+启动
```

### Phase 3: NSH 测试

连接串口终端后执行验证命令：

```bash
# 基础系统检查
nsh> help                    # 查看可用命令
nsh> ps                      # 进程列表
nsh> free                    # 内存状态


### Phase 4: 结果分析

| 现象 | 可能原因 | 下一步 |
|------|---------|--------|
| 编译失败 | 依赖缺失/头文件错误 | 检查 defconfig + menuconfig |
| 烧录后无输出 | flash 地址错误/bootloader 问题 | 检查 esptool 参数 |
| 黑屏 | LVGL 初始化冲突 | kill 冲突进程/vapp |
| 命令不存在 | CONFIG 未启用 | 用 kconfig-tweak 启用 |
| 设备节点不存在 | 驱动未注册 | 检查 board init 代码 |

## 约束

- 配置变更后必须 distclean 再编译
- 烧录前确认端口号和芯片型号
- 编译一次耗时较长（5-17 分钟），避免不必要的全量重编

## 实际耗时参考（来自日志）

| 操作 | 耗时 |
|------|------|
| 增量编译 | ~5 分钟 |
| 全量编译 | ~17 分钟 |
| 烧录 | ~30 秒 |
| NSH 启动 | ~3 秒 |
