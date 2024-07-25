# Сборка ---------------------------------------
# В качестве базового образа для сборки используем gcc:latest
FROM gcc:latest as build

# Установим рабочую директорию для сборки GoogleTest
WORKDIR /gtest_build

# Скачаем все необходимые пакеты и выполним сборку GoogleTest
# Такая длинная команда обусловлена тем, что
# Docker на каждый RUN порождает отдельный слой,
# Влекущий за собой, в данном случае, ненужный оверхед
RUN apt-get update && \
    apt-get install -y \
    libgmock-dev \
    wget \
    ninja-build \
    clang \
    valgrind

# Из apt устанаваливается cmake версии 3.25. На текущий момент минимальная 
# требуемая версия в проекте - 3.28. Для удовлеторвения данному требованию 
# необходимо установить cmake вручную с помощью wget.
RUN rm -rf /var/lib/apt/lists/* \
    && wget https://github.com/Kitware/CMake/releases/download/v3.30.0/cmake-3.30.0-linux-x86_64.sh \
    -q -O /tmp/cmake-install.sh \
    && chmod u+x /tmp/cmake-install.sh \
    && mkdir /opt/cmake-3.30.0 \
    && /tmp/cmake-install.sh --skip-license --prefix=/opt/cmake-3.30.0 \
    && rm /tmp/cmake-install.sh \
    && ln -s /opt/cmake-3.30.0/bin/* /usr/local/bin && \
    cmake -DCMAKE_BUILD_TYPE=Release /usr/src/googletest && \
    cmake --build . && \
    mv ./lib/lib*.a /usr/lib

# Копирование необходимых файлов в приложение
ADD ./ /app/src/

# Установим рабочую директорию для сборки проекта
WORKDIR /app/src

# Сборка всех доступных конфигураций проекта
RUN cmake . --preset pc_debug_clang && \
    cmake --build ./build/pc_debug_clang/

# Запуск ---------------------------------------
# В качестве базового образа используем ubuntu:latest
FROM ubuntu:latest

WORKDIR /gtest_build

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    wget \
    libgmock-dev \
    ninja-build \
    clang \
    valgrind \
    dos2unix

# Из apt устанаваливается cmake версии 3.25. На текущий момент минимальная 
# требуемая версия в проекте - 3.28. Для удовлеторвения данному требованию 
# необходимо установить cmake вручную с помощью wget.
RUN rm -rf /var/lib/apt/lists/* \
    && wget https://github.com/Kitware/CMake/releases/download/v3.30.0/cmake-3.30.0-linux-x86_64.sh \
    -q -O /tmp/cmake-install.sh \
    && chmod u+x /tmp/cmake-install.sh \
    && mkdir /opt/cmake-3.30.0 \
    && /tmp/cmake-install.sh --skip-license --prefix=/opt/cmake-3.30.0 \
    && rm /tmp/cmake-install.sh \
    && ln -s /opt/cmake-3.30.0/bin/* /usr/local/bin && \
    cmake -DCMAKE_BUILD_TYPE=Release /usr/src/googletest && \
    cmake --build . && \
    mv ./lib/lib*.a /usr/lib

# Установим рабочую директорию нашего приложения
WORKDIR /app/src

# Скопируем приложение со сборочного контейнера в рабочую директорию
COPY --from=build /app/src .

RUN ["chmod", "+x", "./docker_tests_entrypoint.sh"]

RUN ["dos2unix", "./docker_tests_entrypoint.sh", "./memcheck.sh"]

# Установим точку входа
ENTRYPOINT ["/usr/bin/bash", "./docker_tests_entrypoint.sh"]
