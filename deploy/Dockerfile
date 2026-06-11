# ============================================
# Dockerfile - C++ demo 应用容器化部署
# 基础镜像: Ubuntu 24.04 (glibc 2.39)
# ============================================

FROM ubuntu:24.04

# 替换为国内镜像源（解决 apt-get update 卡住的问题）
RUN sed -i 's@//.*archive.ubuntu.com@//mirrors.aliyun.com@g' /etc/apt/sources.list.d/ubuntu.sources

# 安装运行时依赖（动态链接的 C++ 程序需要 libstdc++）
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libstdc++6 \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# 创建应用目录
WORKDIR /app

# 拷贝编译好的二进制文件
COPY main .

# 确保可执行
RUN chmod +x main

# 容器启动时执行
ENTRYPOINT ["./main"]


=> [1/6] FROM docker.io/library/ubuntu:24.04@sha256:786a8b558f7be160c6c8c4a54f9a57274f3b4fb1491cf65146521ae77ff1dc54    
