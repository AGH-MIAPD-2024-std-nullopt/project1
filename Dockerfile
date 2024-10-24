FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y software-properties-common && \
    add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get install -y \
    gcc-11=11.4.* \
    g++-11=11.4.* \
    cmake \
    git \
    nodejs \
    npm \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 110 \
    --slave /usr/bin/g++ g++ /usr/bin/g++-11 \
    --slave /usr/bin/gcov gcov /usr/bin/gcov-11

RUN gcc --version && g++ --version

WORKDIR /app

COPY CMakeLists.txt ./
COPY src/ ./src/
COPY includes/ ./includes/
COPY src_html/ ./src_html/

RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make

EXPOSE 8000

CMD ["./build/bin/webserver"]