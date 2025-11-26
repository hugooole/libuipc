#!/bin/bash

set -ex

docker run -it --rm \
    --runtime=nvidia \
    --gpus all \
    -e NVIDIA_VISIBLE_DEVICES=all \
    -e NVIDIA_DRIVER_CAPABILITIES=all \
    -e DISPLAY=$DISPLAY \
    -e QT_X11_NO_MITSHM=1 \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v $HOME/Projects:/Projects \
    zhiguotang5/libuipc:dev
