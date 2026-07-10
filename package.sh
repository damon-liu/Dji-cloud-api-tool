#!/bin/bash
# 一键打包脚本 — 编译 → 部署Qt DLL → 清除凭证 → 打包zip
# 用法: bash package.sh [版本号]

set -e
VERSION="${1:-v1.0}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_mingw"
DEPLOY_DIR="$SCRIPT_DIR/deploy"
OUTPUT="$SCRIPT_DIR/DjiCloudApiTool-${VERSION}.zip"

echo "=== [1/4] 编译项目 ==="
cmake --build "$BUILD_DIR"

echo "=== [2/4] 部署 Qt DLL (windeployqt) ==="
cmake --build "$BUILD_DIR" --target deploy

echo "=== [3/4] 同步到 deploy/ 目录 ==="
cp "$BUILD_DIR/DjiCloudApi.exe" "$DEPLOY_DIR/"
cp "$BUILD_DIR"/*.dll "$DEPLOY_DIR/"
cp -r "$BUILD_DIR"/generic "$BUILD_DIR"/iconengines "$BUILD_DIR"/imageformats \
      "$BUILD_DIR"/networkinformation "$BUILD_DIR"/platforms "$BUILD_DIR"/styles "$BUILD_DIR"/tls \
      "$DEPLOY_DIR/"

# 清除真实凭证，替换为示例模板
cp "$DEPLOY_DIR/config.example.json" "$DEPLOY_DIR/config.json"

echo "=== [4/4] 打包 zip ==="
TMPDIR="$SCRIPT_DIR/DjiCloudApiTool-${VERSION}"
rm -rf "$TMPDIR"
mkdir -p "$TMPDIR"
cp "$DEPLOY_DIR/DjiCloudApi.exe" "$DEPLOY_DIR"/*.dll "$DEPLOY_DIR/config.json" \
   "$DEPLOY_DIR/topic_mappings.json" "$TMPDIR/"
cp -r "$DEPLOY_DIR"/generic "$DEPLOY_DIR"/iconengines "$DEPLOY_DIR"/imageformats \
      "$DEPLOY_DIR"/networkinformation "$DEPLOY_DIR"/platforms "$DEPLOY_DIR"/styles "$DEPLOY_DIR"/tls \
      "$TMPDIR/"
powershell -Command "Compress-Archive -Path '$TMPDIR\*' -DestinationPath '$OUTPUT' -Force"
rm -rf "$TMPDIR"

echo ""
echo "✅ 打包完成: $OUTPUT ($(du -h "$OUTPUT" | cut -f1))"
echo "⚠️  请检查 zip 内 config.json 不含真实凭证后再发布！"
