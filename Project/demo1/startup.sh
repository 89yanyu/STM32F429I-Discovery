#!/bin/bash

qemu-system-gnuarmeclipse \
--board STM32F429I-Discovery \
--mcu STM32F429ZI -s -S \
-d unimp,guest_errors  \
--image demo1.elf 
