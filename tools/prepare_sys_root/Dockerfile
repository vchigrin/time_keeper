FROM debian:bullseye
COPY mobian.list /etc/apt/sources.list.d/
COPY mobian.gpg /etc/apt/trusted.gpg.d/
RUN dpkg --add-architecture arm64
RUN apt update
RUN apt install -y \
  libhandy-1-dev:amd64 \
  libgtkmm-3.0-dev:amd64 \
  libsqlite3-dev:arm64 \

