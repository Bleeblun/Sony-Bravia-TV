#!/bin/bash
# sample command for build
# $ export ARCH=arm
# $ export CROSS_COMPILE=arm-linux-gnueabi-
# $ make bravia_defconfig
# $ make modules
# $ make uImage
# 
make bravia_defconfig
make modules
make uImage
