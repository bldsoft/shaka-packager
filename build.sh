#!/bin/sh
GYP_DEFINES='clang=0 use_experimental_allocator_shim=0 use_allocator=none libpackager_type=shared_library' gclient runhooks
ninja -C out/Release