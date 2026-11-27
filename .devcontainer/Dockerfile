FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        cmake \
        git \
        wget \
        libcap2-bin \
        sudo \
        iproute2 \
        dnsmasq \
        curl \
        socat \
        tcpdump \
        ethtool \
        g++-riscv64-linux-gnu \
        g++-arm-linux-gnueabihf \
        clang-18 \
        clang-format-18 \
        clang-tidy-18 \
        && rm -rf /var/lib/apt/lists/*

# Update alternatives to use clang-18
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-18 100 && \
    update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-18 100

# Use the default "ubuntu" user (UID/GID 1000) that ships with ubuntu:24.04, with passwordless sudo
ENV USERNAME=ubuntu
RUN echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME && \
    chmod 0440 /etc/sudoers.d/$USERNAME

# Mount point for the host's ~/.claude (ClaudeSupport config)
RUN mkdir -p /home/$USERNAME/.claude && chown -R $USERNAME:$USERNAME /home/$USERNAME/.claude

USER $USERNAME

CMD ["bash"]
