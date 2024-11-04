#!/bin/bash

GIT_REPO="$(cat repo)"

while true; do
    clear
    echo "Выберите действие:"
    echo "1. Pull"
    echo "2. Commit и Push"
    echo "3. Exit"
    read -p "Введите ваш выбор (1/2/3): " choice

    if [[ "$choice" == "1" ]]; then
        echo "Выполняется pull из репозитория..."
        git pull
        if [[ $? -ne 0 ]]; then
            echo "Ошибка при выполнении git pull."
			sleep 5
            break
        fi
        clear
        echo "Pull выполнен успешно."
        sleep 2

    elif [[ "$choice" == "2" ]]; then
        read -p "Введите текст коммита: " commit_msg

        echo "Значение commit_msg: \"$commit_msg\""

        git add .
        git commit -m "$commit_msg"
        if [[ $? -ne 0 ]]; then
            echo "Ошибка при выполнении git commit."
			sleep 5
            break
        fi
        clear
        echo "Выполняется push в репозиторий..."
        git push "$GIT_REPO" master
        if [[ $? -ne 0 ]]; then
            echo "Ошибка при выполнении git push."
			sleep 5
            break
        fi
        echo "Push выполнен успешно."
        sleep 2
	elif [[ "$choice" == "3" ]]; then
		echo "Завершение работы через 5 секунд..."
		sleep 5
		exit 0
    else
        echo "Неверный выбор."
        echo "Перезапуск программы..."
        sleep 2
    fi
	sleep 5
done