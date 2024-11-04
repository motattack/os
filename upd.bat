@echo off
chcp 65001 >nul

setlocal enabledelayedexpansion
:main
cls
set /p GIT_REPO=<repo

echo Выберите действие:
echo 1. Pull
echo 2. Commit и Push
echo 3. Exit
set /p "choice=Введите ваш выбор (1|2|3): "

if "%choice%"=="1" (
    echo Выполняется pull из репозитория...
    git pull
    if errorlevel 1 (
        echo Ошибка при выполнении git pull.
		timeout /t 5 >nul
        goto main
    )
    cls
    echo Pull выполнен успешно.
) else if "%choice%"=="2" (
    set /p "commit_msg=Введите текст коммита: "

    timeout /t 5 >nul

    echo Значение commit_msg: "!commit_msg!"

    git add .
    git commit -m "!commit_msg!"
    if errorlevel 1 (
        echo Ошибка при выполнении git commit.
		timeout /t 5 >nul
        goto main
    )
    
    echo Выполняется push в репозиторий...
    git push %GIT_REPO% master
    if errorlevel 1 (
        echo Ошибка при выполнении git push.
		timeout /t 5 >nul
        goto main
    )
    echo Push выполнен успешно.
) else if "%choice%"=="3" (
	echo Завершение работы через 5 секунд...
	timeout /t 5 >nul
	exit /b 0
) else (
    echo Неверный выбор.
    echo Перезапуск программы...
    timeout /t 2 >nul
)
timeout /t 5 >nul
goto main

endlocal
