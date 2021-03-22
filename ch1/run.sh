#!/bin/bash
cwd=$(pwd)

qemu-system-x86_64 \
  -drive if=pflash,file=$cwd/OVMF_CODE.fd \
  -drive if=pflash,file=$cwd/OVMF_VARS.fd \
  -hda disk.img
