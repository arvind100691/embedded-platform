FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    python3 \
    python3-pip \
    pipx \
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    gdb-multiarch \
    openocd \
    clang \
    clang-format \
    clang-tidy \
    cppcheck \
    gcovr \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

ENV PATH="/root/.local/bin:${PATH}"

RUN pipx install cmake-format --include-deps
RUN pipx ensurepath

CMD ["/bin/bash"]