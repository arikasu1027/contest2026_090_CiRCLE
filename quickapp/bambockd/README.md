# 竹知了 (Bamboo Cicada)

摇一摇就叫的竹知了小程序。

## 功能

- 屏幕显示"竹知了"文字
- 通过加速度计检测摇晃动作
- 当加速度超过阈值时播放蝉鸣音效
- 实时显示加速度数据和状态

## 依赖模块

- `system.sensor` - 加速度计传感器
- `system.audio` - 音频播放

## 音频文件

需要在设备上放置音频文件：
```
/data/audio/cicada.wav
```

支持 WAV 格式。

## 阈值调整

默认阈值为 2500（加速度向量模长）。可通过修改 `src/pages/index/index.ux` 中的 `threshold` 值调整灵敏度。
