#!/bin/bash

PROJECT_DIR="$(pwd)"
BUILD_DIR="$PROJECT_DIR/build"
CMAKE_FILE="$PROJECT_DIR/CMakeLists.txt"
EXE_PATH="$BUILD_DIR/main"

if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR" || exit

cmake "$CMAKE_FILE"
if [ $? -ne 0 ]; then
    echo "Ошибка конфигурации CMake."
    exit 1
fi

make
if [ $? -ne 0 ]; then
    echo "Ошибка сборки."
    exit 1
fi

echo "Запуск программы через 5 секунд..."
sleep 5
clear

if [ -f "$EXE_PATH" ]; then
    "$EXE_PATH"
else
    echo "Исполняемый файл не найден."
fi