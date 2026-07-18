# ============================================
# DJI-CLOUD-API 监控客户端 构建配置
# ============================================
cmake_minimum_required(VERSION 3.10)
project(DjiCloudApi VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# 查找 Qt6
find_package(Qt6 COMPONENTS Core Widgets Mqtt REQUIRED)

# 源文件
set(SOURCES
    src/main.cpp
    src/core/ConfigStore.cpp
    src/core/DeviceManager.cpp
    src/core/TopicManager.cpp
    src/mqtt/MqttClientManager.cpp
    src/ui/MainWindow.cpp
    src/ui/DeviceTreeWidget.cpp
    src/ui/OsdPanel.cpp
    src/ui/RawJsonPanel.cpp
    src/ui/PublishPanel.cpp
    src/ui/ConfigDialog.cpp
    src/ui/TopicEditDialog.cpp
    src/ui/TopicListWidget.cpp
    src/ui/TopicParsePanel.cpp
    src/core/DockCommand.cpp
    src/core/DockCommandExecutor.cpp
    src/ui/DockControlPanel.cpp
)

set(HEADERS
    src/core/DeviceInfo.h
    src/core/OsdData.h
    src/core/ConfigStore.h
    src/core/DeviceManager.h
    src/core/TopicManager.h
    src/mqtt/MqttClientManager.h
    src/ui/MainWindow.h
    src/ui/DeviceTreeWidget.h
    src/ui/OsdPanel.h
    src/ui/RawJsonPanel.h
    src/ui/PublishPanel.h
    src/ui/ConfigDialog.h
    src/ui/TopicEditDialog.h
    src/ui/TopicListWidget.h
    src/core/TopicMapping.h
    src/ui/TopicParsePanel.h
    src/core/DockCommand.h
    src/core/DockCommandExecutor.h
    src/ui/DockControlPanel.h
    src/ui/DockControlDialog.h
)

add_executable(DjiCloudApi ${SOURCES} ${HEADERS})

target_include_directories(DjiCloudApi PRIVATE
    src
    src/core
    src/mqtt
    src/ui
)

target_link_libraries(DjiCloudApi
    Qt6::Core
    Qt6::Widgets
    Qt6::Mqtt
)

# 复制默认配置文件到构建目录
configure_file(
    ${CMAKE_SOURCE_DIR}/src/resources/config.json
    ${CMAKE_BINARY_DIR}/config.json
    COPYONLY
)

# 复制 topic 映射配置文件到构建目录
configure_file(
    ${CMAKE_SOURCE_DIR}/config/topic_mappings.json
    ${CMAKE_BINARY_DIR}/topic_mappings.json
    COPYONLY
)

# 编译器标志
if(MSVC)
    target_compile_options(DjiCloudApi PRIVATE /utf-8)
else()
    target_compile_options(DjiCloudApi PRIVATE -fexec-charset=UTF-8)
endif()

# ============================================
# Windows 部署: windeployqt 自动打包 (可选)
# 用法: cmake --build . --target deploy
# ============================================
if(WIN32)
    add_custom_target(deploy
        COMMAND ${CMAKE_COMMAND} -E echo "=== Running windeployqt ==="
        COMMAND windeployqt
            --no-translations
            "$<TARGET_FILE:DjiCloudApi>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/src/resources/config.json"
            "${CMAKE_BINARY_DIR}/config.json"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/config/topic_mappings.json"
            "${CMAKE_BINARY_DIR}/topic_mappings.json"
        # 强制清除 deploy/ 目录中的真实凭证，防止打包泄露
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/deploy/config.example.json"
            "${CMAKE_SOURCE_DIR}/deploy/config.json"
        DEPENDS DjiCloudApi
        COMMENT "Deploying Qt DLLs and config for Windows distribution"
    )
endif()
