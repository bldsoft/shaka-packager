#!/bin/sh
GYP_DEFINES='clang=0 libpackager_type=shared_library' gclient runhooks
ninja -C out/Release
ninja -C out/Debug