#!/bin/bash

qemu-system-gnuarmeclipse \
--verbose \
--board STM32F429I-Discovery \
-s \
-d unimp,guest_errors  \
--image demo1.elf \
--semihosting-config enable=on,target=native
