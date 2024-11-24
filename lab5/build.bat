@echo off
chcp 65001 >nul

set "PROJECT_DIR=%cd%"
set "BUILD_DIR=%PROJECT_DIR%\build"
set "CMAKE_FILE=%PROJECT_DIR%\CMakeLists.txt"

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

cd "%BUILD_DIR%"


cmake -G "MinGW Makefiles" "%CMAKE_FILE%"
if %errorlevel% neq 0 (
    echo Ошибка конфигурации CMake.
	cd ..
    exit /b %errorlevel%
)

mingw32-make
if %errorlevel% neq 0 (
    echo Ошибка сборки.
	cd ..
    exit /b %errorlevel%
)

echo Запуск программы через 5 секунд...
timeout /t 5 >nul
cls

set "EXE_PATH=%BUILD_DIR%\main.exe"
if exist "%EXE_PATH%" (
    "%EXE_PATH%"
) else (
    echo Исполняемый файл не найден.
)

cd ..
pause