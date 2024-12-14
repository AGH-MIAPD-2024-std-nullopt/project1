FROM ubuntu:22.04

RUN apt update && \
    apt install -y \
    gcc=4:11.2.* \
    g++=4:11.2.* \
    cmake=3.22.*

RUN useradd webserver_executor

WORKDIR /app

RUN chown -R webserver_executor:webserver_executor /app

COPY CMakeLists.txt ./
COPY includes/ ./includes/
COPY src/ ./src/
COPY src_html/ ./src_html/

RUN cmake . && \
    cmake --build .

EXPOSE 8080

CMD ["./build/bin/webserver"]