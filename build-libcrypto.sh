#!/usr/bin/env bash
(
export ANDROID_API=28
export ANDROID_ABI=23
export ANDROID_CPU=arm64-v8a
source setenv-android.sh
cd ./include/cryptopp
make -f GNUmakefile-cross distclean
make -f GNUmakefile-cross static dynamic
)