# 大疆机场3视频流播放方案

> 日期：2026-07-22
> 目标：在 Qt 6 桌面客户端中嵌入大疆机场3推送的三路视频流播放
> 前提：用户已自行部署流媒体服务器（RTMP/RTSP/WebRTC），本项目仅做客户端拉流播放

---

## 1. 大疆机场3视频能力概述

大疆机场3（DJI Dock 3）搭载经纬 Matrice 4D / 4TD 无人机，支持多路视频同时推流到第三方流媒体服务器。

### 1.1 摄像头列表

| 序号 | 摄像头 | 说明 |
|------|--------|------|
| 1 | FPV 摄像头 | 飞行第一视角 |
| 2 | 广角相机 | 24mm 等效，48MP |
| 3 | 中长焦相机 | 48mm 等效（3x），48MP |
| 4 | 长焦相机 | 168mm 等效（7x），48MP |
| 5 | 红外热成像（4TD） | 640×512 |

### 1.2 推流协议

| 协议 | 枚举值 | 延迟 | 说明 |
|------|--------|------|------|
| **Agora** | 0 | <1s | 声网私有协议 |
| **RTMP** | 1 | 2~5s | 兼容性最好，开源服务器多 |
| **GB28181** | 3 | 中 | 国标安防协议 |
| **WebRTC/WHIP** | 4 | <500ms | 极低延迟 |

### 1.3 推流流程

```
无人机 ──AirLink──> DJI Dock（网关） ──推流──> 第三方流媒体服务器 <──拉流── 本项目客户端
                         │
                         └── MQTT（控制信令，本项目不涉及视频推送控制）
```

- 推流控制（live_start_push / live_stop_push / 镜头切换 / 画质调整）通过 MQTT 信令完成
- 视频数据直接从机场推送到流媒体服务器（RTMP/RTSP/WebRTC）
- **本项目仅做客户端拉流播放**，不控制推流

---

## 2. 方案前提

> **用户已部署流媒体服务器**，本项目不集成、不部署、不管理任何流媒体服务器。

本项目只需从用户提供的播放 URL 拉流并渲染到 Qt 窗口中。需支持同时播放 3 路视频流（对应 `VideoStreamWindow` ×3）。

---

## 3. 客户端播放方案对比

### 3.1 方案一：libVLC（推荐）

VLC 的 libVLC 库—— Qt 集成最成熟的方案。

| 维度 | 评价 |
|------|------|
| 协议覆盖 | RTMP / RTSP / HLS / SRT / UDP 全支持 |
| 解码能力 | H.264 / H.265 硬解开箱即用 |
| 多路播放 | 每个 `libvlc_media_player` 实例独立解码，互不干扰 |
| Qt 集成 | 通过 `libvlc_media_player_set_hwnd()` 将视频渲染到任意 `QWidget::winId()` |
| 打包体积 | libvlc.dll + libvlccore.dll + plugins/ ≈ 50MB（裁剪后） |
| 成熟度 | VLC 20 年历史，Bug 极少 |
| 许可证 | LGPL v2.1 |

**集成方式：**

```cpp
// 创建 VLC 实例（全局共享一个 libvlc_instance_t）
libvlc_instance_t* vlc = libvlc_new(0, nullptr);

// 每个 VideoStreamWindow 持有一个播放器
libvlc_media_player_t* mp = libvlc_media_player_new(vlc);
libvlc_media_player_set_hwnd(mp, (void*)widget->winId());

// 播放
libvlc_media_t* media = libvlc_media_new_location(vlc, "rtmp://server/live/stream1");
libvlc_media_player_set_media(mp, media);
libvlc_media_player_play(mp);
```

**优点：** 协议兼容性最强，硬解支持好，Qt 集成简单，多路同时播放无压力。

**缺点：** 需要随程序分发 libVLC 动态库（约 50MB），LGPL 许可证需注意合规。

---

### 3.2 方案二：Qt Multimedia（QMediaPlayer + QVideoWidget）

Qt 6 内置的多媒体模块。

| 维度 | 评价 |
|------|------|
| 协议覆盖 | 取决于系统后端（Windows: WMF / Linux: GStreamer） |
| 解码能力 | 系统后端决定 |
| 多路播放 | 多实例可行，但性能取决于后端 |
| Qt 集成 | `setVideoOutput(QVideoWidget*)` 原生 API |
| 打包体积 | 0（Qt 自带） |

**集成方式：**

```cpp
QMediaPlayer* player = new QMediaPlayer;
QVideoWidget* videoWidget = new QVideoWidget;
player->setVideoOutput(videoWidget);
player->setSource(QUrl("rtmp://server/live/stream1"));
player->play();
```

**优点：** 零额外依赖，API 简洁。

**缺点：**
- Windows WMF 后端不原生支持 RTMP，需额外安装 LAV Filters 或改用 GStreamer
- 后端差异大，跨平台一致性差
- 多路同时播放时性能不可控
- 协议支持有限，不支持 RTSP / WebRTC

---

### 3.3 方案三：FFmpeg 解码 + QOpenGLWidget 渲染

手写解码管线，完全自主可控。

| 维度 | 评价 |
|------|------|
| 协议覆盖 | 全协议（RTMP/RTSP/SRT 等） |
| 解码能力 | 完全可控，可配置硬解 |
| 多路播放 | 支持，需自行管理线程 |
| Qt 集成 | YUV → OpenGL 纹理，需手写 `QOpenGLWidget` |
| 打包体积 | ffmpeg DLLs ≈ 30MB |

**优点：** 延迟最低（可 <500ms），完全自定义缓冲策略和 OSD 叠加。

**缺点：**
- 开发量大（3~5 周），需处理解封装、解码、音视频同步、seek、硬解适配
- FFmpeg API 不稳定，版本升级维护成本高
- 多路并行解码需自行管理线程和资源

---

### 3.4 方案四：QWebEngineView 内嵌 WebRTC

通过 Qt WebEngine 加载 Web 页面播放 WebRTC 流。

| 维度 | 评价 |
|------|------|
| 协议覆盖 | 仅 WebRTC |
| 解碼能力 | 浏览器引擎处理 |
| 多路播放 | 支持，但资源消耗大 |
| Qt 集成 | `QWebEngineView` 直接嵌入 |
| 打包体积 | Qt WebEngine ≈ 200MB |

**优点：** 开发量低，WebRTC 延迟极低。

**缺点：**
- 打包体积巨大（WebEngine ≈ 200MB）
- 仅适用于 WebRTC 协议
- 3 路 WebEngine 实例资源消耗过高
- 与原生 Qt 信号/槽交互不便

---

## 4. 方案对比总结

| 维度 | libVLC | Qt Multimedia | FFmpeg+GL | WebEngine |
|------|--------|---------------|-----------|-----------|
| RTMP/RTSP 支持 | 完整 | 差（Windows） | 完整 | 不支持 |
| WebRTC 支持 | 不支持 | 不支持 | 可实现 | 原生 |
| 多路播放 | 优秀 | 一般 | 需自管理 | 重 |
| Qt 集成难度 | 低 | 极低 | 高 | 低 |
| 开发工作量 | 1~2 天 | 0.5 天 | 3~5 周 | 0.5 天 |
| 打包体积 | +50MB | +0 | +30MB | +200MB |
| 硬解支持 | 开箱即用 | 系统决定 | 需适配 | 浏览器处理 |
| **推荐** | **首选** | 备选 | 不推荐 | 不推荐 |

---

## 5. 推荐方案：libVLC

### 5.1 选择理由

1. **协议覆盖最广** — RTMP/RTSP/HLS/SRT 全支持，无论用户部署哪种流媒体服务器都能播放
2. **多路并行无压力** — 3 个独立 `libvlc_media_player` 实例，各自硬解，互不干扰
3. **Qt 集成简单** — 通过 HWND 直接嵌入到 `VideoStreamWindow::mVideoArea`
4. **成熟稳定** — VLC 20 年历史，生产环境验证充分
5. **开发量可控** — 1~2 天即可完成集成

### 5.2 后续扩展性

若未来需要 WebRTC 低延迟播放，可保留 `VideoStreamWindow` 的 `setStreamUrl()` 接口不变，内部切换为 FFmpeg WebRTC 客户端或 WebEngine 实现，UI 层零改动。

---

## 6. 实施计划

### 6.1 当前已完成

- `VideoStreamWindow` ×3 — 三个 480×270 视频窗口，垂直排列在主窗口左侧
- 一键起飞成功后自动弹出，MQTT 断连时自动隐藏
- `setStreamUrl(QString)` 接口预留

### 6.2 待实施

1. **引入 libVLC**
   - CMake 链接 `libvlc` 和 `libvlccore`
   - 随程序分发 VLC 运行时 DLL（约 50MB，裁剪后）

2. **改造 VideoStreamWindow**
   - 内部创建 `libvlc_media_player`，渲染到 `mVideoArea->winId()`
   - `setStreamUrl()` 实现实际播放逻辑
   - 析构时释放 VLC 资源

3. **集成 VLC 实例管理**
   - 全局共享一个 `libvlc_instance_t`（VLC 建议单例）
   - 每个 `VideoStreamWindow` 创建独立 `libvlc_media_player`

4. **播放控制（可选）**
   - 播放/暂停/停止
   - 音量控制
   - 截图
   - 全屏切换

### 6.3 VideoStreamWindow 改造示意

```cpp
// VideoStreamWindow.h 新增成员
struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_media_t;

class VideoStreamWindow : public QWidget {
    // ...
    void setStreamUrl(const QString& url);
    void stopStream();
private:
    int mIndex;
    QLabel* mTitleLabel;
    QWidget* mVideoArea;
    QLabel* mPlaceholderLabel;
    
    // libVLC
    libvlc_instance_t*      mVlcInstance = nullptr;
    libvlc_media_player_t*  mVlcPlayer = nullptr;
    libvlc_media_t*         mVlcMedia = nullptr;
    bool mPlaying = false;
};
```

---

## 7. 相关链接

- DJI Cloud API 直播文档：https://developer.dji.com/doc/cloud-api-tutorial/en/feature-set/dock-feature-set/dock-livestream.html
- libVLC C API 文档：https://www.videolan.org/developers/vlc/doc/doxygen/html/group__libvlc.html
- vlc-qt（C++ 封装）：https://github.com/vlc-qt/vlc-qt
- 大疆机场3产品页：https://enterprise.dji.com/cn/dock-3/specs
