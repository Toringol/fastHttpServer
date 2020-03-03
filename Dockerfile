FROM ubuntu:latest
MAINTAINER "Shepelev Sergey"
WORKDIR /opt/httpd
COPY . .
RUN apt-get update && yes | \
    apt-get install "libevent-dev" "zlib1g-dev" "cmake"
RUN ./main.sh
RUN useradd httpd
EXPOSE 80
ENTRYPOINT ["./bin/HighloadServer", "/etc/httpd.conf"]

