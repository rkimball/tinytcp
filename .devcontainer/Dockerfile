FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    && rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/llvm/llvm-project/releases/download/llvmorg-16.0.4/clang+llvm-16.0.4-x86_64-linux-gnu-ubuntu-22.04.tar.xz \
    && tar -xvf clang+llvm-16.0.4-x86_64-linux-gnu-ubuntu-22.04.tar.xz \
    && mv clang+llvm-16.0.4-x86_64-linux-gnu-ubuntu-22.04 /usr/local/clang_16.0.4 \
    && rm clang+llvm-16.0.4-x86_64-linux-gnu-ubuntu-22.04.tar.xz

ENV PATH="/usr/local/clang_16.0.4/bin:${PATH}"

# # Install clang
# RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - \
#     && apt-add-repository "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-13 main" \
#     && apt-get update && apt-get install -y \
#     clang-13 \
#     clang-format-13 \
#     clang-tidy-13 \
#     lld-13 \
#     && rm -rf /var/lib/apt/lists/*

COPY . /workspace

CMD bash
