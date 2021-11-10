#!/bin/bash
set -e
CONTAINER_NAME=tmp-mobian-sysroot-container
CONTAINER_IMAGE=tmp:mobian-sysroot
SCRIPT_DIR=$(dirname $0)
TARGET_DIR=$1
if [ -z $TARGET_DIR ]; then
  TARGET_DIR=$(pwd)/mobian_sysroot
fi
echo "Preparing sysroot in $TARGET_DIR"
rm -rf ${TARGET_DIR}/*
mkdir -p ${TARGET_DIR}
echo "Building Docker image..."
docker build --tag ${CONTAINER_IMAGE} ${SCRIPT_DIR}
echo "Image built, exporing..."
docker run --name ${CONTAINER_NAME} ${CONTAINER_IMAGE}
docker export ${CONTAINER_NAME} | tar -x -C ${TARGET_DIR}
docker rm ${CONTAINER_NAME}
