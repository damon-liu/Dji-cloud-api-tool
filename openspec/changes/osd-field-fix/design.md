# Design: 字段修复

## 修改内容

| 文件 | 改动 |
|------|------|
| OsdData.h | cover_state: QString→int, dock_inside_temp→key="temperature", 解析 position_state.gps_number |
| OsdPanel.cpp | coverText() 适配 int 输入 |
