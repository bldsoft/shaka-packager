#!/bin/sh
GYP_DEFINES='clang=0 use_experimental_allocator_shim=0 use_allocator=none --args=is_debug=true' gclient runhooks
ninja -C out/Release
ninja -C out/Debug