FROM ubuntu:24.04

SET (CMAKE_EXPORT_COMPILE_COMMANDS=ON)

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        cmake \
        git \
        wget \
        libcap2-bin \
        clang-18 \
        clang-format-18 \
        clang-tidy-18 \
        && rm -rf /var/lib/apt/lists/*

# Update alternatives to use clang-18
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-18 100 && \
    update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-18 100

# Grant CAP_NET_RAW capability to your executable
# RUN setcap cap_net_raw+ep /path/to/your/executable

# COPY . /workspace

CMD ["bash"]
