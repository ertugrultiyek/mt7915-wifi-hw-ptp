FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# OpenWrt & MTK SDK build bağımlılıkları
RUN apt-get update && apt-get install -y \
    build-essential \
    asciidoc \
    binutils \
    bzip2 \
    gawk \
    gettext \
    git \
    libncurses5-dev \
    libz-dev \
    patch \
    python3 \
    python2 \
    unzip \
    zlib1g-dev \
    libssl-dev \
    subversion \
    flex \
    bison \
    file \
    wget \
    rsync \
    sudo \
    cmake \
    vim \
    quilt \
    bc \
    cpio \
    && rm -rf /var/lib/apt/lists/*

# Python alias (Eski vendor scriptleri için python -> python2 veya python3)
RUN ln -sf /usr/bin/python3 /usr/bin/python

# OpenWrt root yetkisiyle derlenmez; non-root user şart
RUN useradd -m -s /bin/bash builder && \
    echo "builder ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER builder
WORKDIR /workspace

CMD ["/bin/bash"]