FROM debian:bullseye
COPY mobian.list /etc/apt/sources.list.d/
COPY mobian.gpg /etc/apt/trusted.gpg.d/
RUN dpkg --add-architecture arm64
RUN apt update
RUN apt install -y \
  libboost-dev:arm64 \
  libgtest-dev:arm64 \
  libgtkmm-3.0-dev:arm64 \
  libhandy-1-dev:arm64 \
  libsqlite3-dev:arm64 \
  libstdc++-9-dev:arm64
