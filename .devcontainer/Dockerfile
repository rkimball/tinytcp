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
        clang-18 \
        clang-format-18 \
        clang-tidy-18 \
        && rm -rf /var/lib/apt/lists/*

# Update alternatives to use clang-18
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-18 100 && \
    update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-18 100

# Create a non-root user "dev" with passwordless sudo.
# UID/GID default to 1000 and can be overridden with --build-arg to match the host.
# The ubuntu:24.04 image ships a default "ubuntu" user at UID/GID 1000; remove it first.
ARG USER_UID=1000
ARG USER_GID=1000
RUN (userdel -r ubuntu 2>/dev/null || true) && \
    (groupdel ubuntu 2>/dev/null || true) && \
    groupadd --gid ${USER_GID} dev && \
    useradd --create-home --shell /bin/bash --uid ${USER_UID} --gid ${USER_GID} dev && \
    echo "dev ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/dev && \
    chmod 0440 /etc/sudoers.d/dev

USER root
RUN mkdir -p /home/dev/.claude && chown -R dev:dev /home/dev/.claude
USER dev

CMD ["bash"]
