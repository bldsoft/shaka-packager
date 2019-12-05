#!/bin/sh
GYP_DEFINES='clang=0' gclient runhooks
ninja -C out/Release
ninja -C out/Debug