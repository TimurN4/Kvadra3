--gRPC
Я его сам собирал из исходников

1) Настройка переменных окружения
echo 'export MY_INSTALL_DIR=$HOME/.local' >> ~/.bashrc (создаем переменную(она содержит путь до папки где мы будем хранить бинарники для gRPC) которую добавляем в специальную папку которую сначала читают)

mkdir -p $MY_INSTALL_DIR (физически ее создаем)

echo 'export PATH="$MY_INSTALL_DIR/bin:$PATH"' >> ~/.bashrc (добавляем в PATH новый путь - наша репо с бинарниками gRPC, теперь cmake будет находит gRPC когда мы будем писать find_package и т.д)

source ~/.bashrc (применяем изменения)

2) Установка необходимых инструментов

sudo apt update
sudo apt install -y build-essential autoconf libtool pkg-config git

3) Клонирование репозитория gRPC
Исходники я клонировал в корень проекта
git clone --recurse-submodules -b v1.78.1 --depth 1 --shallow-submodules https://github.com/grpc/grpc

4) Сборка и установка gRPC(а собирал бинарники в папке .local которая рядом с системными)
-cd grpc
-mkdir -p cmake/build
-cd cmake/build
-cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_CXX_STANDARD=17 \
      -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
      ../..
-make -j 4
-make install
-cd ../..

5) Проверка установки (сборка примера)
cd examples/cpp/helloworld
mkdir -p cmake/build
cd cmake/build
cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..
make -j 4

6) 
В одном терминале WSL:
cd ~/grpc/examples/cpp/helloworld/cmake/build
./greeter_server

В другом терминале:
cd ~/grpc/examples/cpp/helloworld/cmake/build
./greeter_client


--BOOST

Перед этим я обновил свой conan до 2 версии

Также мой conan думал что у меня стоит gcc 10.5(раньше он у меня стоял пока я не обновил его) и отстутствовал профиль

Пришлось создать профиль 

# conan profile detect - он содержит все данные о том какой компилятор, архитектура процессора и т.д

# conan profile show - можно проверить какой сейчас профиль и какие данные 


Я его подключал с помощью conan

# Находясь внутри папки build
0) Создал в корне проекта conanfile.txt

1) conan install .. --output-folder=. --build=missing(скачивает boost и кжширует ее в .conan - там содержится boost)

2) cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release

# -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake - это нужно чтобы связать cmake и conan
Обьяснение:

if(EXISTS .../conan_toolchain.cmake): Проверяет, создал ли Conan специальный файл настроек (toolchain-файл). Этот файл обычно появляется после того, как вы запустили команду conan install.

${CMAKE_BINARY_DIR}: Это путь к вашей папке сборки (обычно build). Именно там Conan оставляет свои инструкции.

include(...): Если файл найден, CMake выполняет его. Внутри этого файла прописаны пути к компиляторам, библиотекам и флагам, которые специфичны для окружения, созданного Conan.Зачем это нужно?
Без этой вставки CMake не будет знать, где искать библиотеки, которые вы прописали в conanfile.txt или conanfile.py.
Это работает так:
-Вы запускаете conan install ..
-Conan скачивает библиотеки (например, OpenSSL или Boost) и генерирует файл conan_toolchain.cmake.
-Вы запускаете CMake. Благодаря этому коду CMake «подхватывает» настройки от Conan.
-Теперь вы можете просто писать find_package(название_библиотеки REQUIRED), и всё будет работать.

# P.S:
На самом деле чтобы связать cmake и conan нужно либо вручную указать в терминале флаг -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

либо

добавить в cmakelists.txt этот блок

"""
if(EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
    include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
endif()
"""

# -DCMAKE_BUILD_TYPE=Release - необходимо для сборки
Conan сгенерировал BoostConfig.cmake, который требует явно указать CMAKE_BUILD_TYPE (Debug или Release).




