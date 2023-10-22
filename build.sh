#!/bin/sh

set -e

mkdir -p build
mkdir -p build/cppcheck

cppcheck . -q -i filex -i FlashDB -i netxduo -i threadx -i build -i stm32h5xx_hal_driver -f --cppcheck-build-dir=build/cppcheck
cppcheck . --enable=style --suppress=constParameterPointer --suppress=cstyleCast --suppress=noExplicitConstructor -q -i lwjson -i mkfat -i filex -i FlashDB -i netxduo -i threadx -i build -i stm32h5xx_hal_driver -f --cppcheck-build-dir=build/cppcheck

build_type="Ninja"

cd build
rm -rf lightkraken2_debug*
mkdir -p lightkraken2_debug
cd lightkraken2_debug
cmake -Wno-deprecated -G "$build_type" -DCMAKE_SYSTEM_NAME=Generic -DCMAKE_TOOLCHAIN_FILE=../../cmake/arm-gcc-toolchain.cmake -DCMAKE_BUILD_TYPE=Debug ../..
cmake --build .
cd ../..

cd build
rm -rf lightkraken2_release*
mkdir -p lightkraken2_release
cd lightkraken2_release
cmake -Wno-deprecated -G "$build_type" -DCMAKE_SYSTEM_NAME=Generic -DCMAKE_TOOLCHAIN_FILE=../../cmake/arm-gcc-toolchain.cmake -DCMAKE_BUILD_TYPE=Release  ../..
cmake --build .
cd ../..
