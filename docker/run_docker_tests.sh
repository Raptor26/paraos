#!/bin/bash

echo "Запуск сборки docker."
sh ./dbuild.sh
echo "Запуск docker conatiner."
sh ./drunner.sh

read  -p  "Нажмите клавишу [Enter] чтобы завершить выполнение скрипта"
