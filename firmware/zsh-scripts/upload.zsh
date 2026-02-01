#!/bin/zsh

make clean
make

# Upload with custom .cfg command
make program OOCD_SCRIPT=ciaa-nxp-custom.cfg

