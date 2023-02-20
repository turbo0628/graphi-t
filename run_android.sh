#!/bin/bash

AppName=$1

mkdir -p build-android-aarch64

pushd "build-android-aarch64"
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" -DANDROID_ABI="arm64-v8a" -DANDROID_PLATFORM=android-32 -G "Ninja" ..
cmake --build . -t $AppName
ret=$?
if [ $ret -eq 0 ]; then
adb push ./assets /data/local/tmp/graphi-t
adb push ./bin /data/local/tmp/graphi-t
adb shell chmod 777 /data/local/tmp/graphi-t/bin/$AppName
adb shell "cd /data/local/tmp/graphi-t/bin && ./$AppName"
else
echo "Failed to build binary, aborted."
fi
popd
