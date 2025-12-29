FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        cmake \
        git \
        wget \
        libcap2-bin \
        libcap-dev \
        clang-18 \
        clang-format-18 \
        clang-tidy-18 \
        && rm -rf /var/lib/apt/lists/*

# Update alternatives to use clang-18
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-18 100 && \
    update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-18 100

ENV USERNAME=ubuntu
RUN \
    apt-get update && \
    apt-get install -y sudo && \
    echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME && \
    chmod 0440 /etc/sudoers.d/$USERNAME

# Allow passwordless setcap for the ubuntu user (for setting capabilities on test binaries)
# This lets the build system grant CAP_NET_RAW to test executables without interactive sudo
RUN echo "$USERNAME ALL=(root) NOPASSWD: /usr/sbin/setcap" >> /etc/sudoers.d/$USERNAME

USER $USERNAME

CMD ["bash"]
