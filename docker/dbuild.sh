# Copyright (c) 2024 Stilsoft
# Distributed under the MIT License
# Author Vyhodcev Egor

#!/bin/bash

# После пересборки образа для тестирования предыдущий собранный образ помечается как "Dangling" - его слои больше 
# не используются для создания контейнеров. Такой образ можно спокойно удалять для экономии места на диске.
docker images --quiet --filter=dangling=true | xargs --no-run-if-empty docker rmi

# Символы ".." после слова build означают, что контекст сборки расширяется на 1 уровень вложенности вверх (весь проект).
docker build .. -t docker_test_paraos:1.0 -f ../Dockerfile
