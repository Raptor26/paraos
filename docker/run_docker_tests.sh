# Copyright (c) 2024 Stilsoft
# Distributed under the MIT License
# Author Vyhodcev Egor

#!/bin/bash

RED=$(tput setaf 1)
NC=$(tput sgr0)

build_script="sh ./dbuild.sh"

echo "Запуск сборки docker."

if eval "$build_script" == 0; then
    echo $?
    echo "Запуск docker conatiner."
    sh ./drunner.sh
else
    echo "${RED}ERROR: Произошла ошибка во время сборки Docker образа!"
fi

read  -p  "Нажмите клавишу [Enter] чтобы завершить выполнение скрипта"
