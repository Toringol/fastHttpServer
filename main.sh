#!/bin/bash

set -e

pathToScript=`readlink -e "$0"`    #Абсолютный путь до файла скрипта
basePath=`dirname "$pathToScript"` #Абсолютный путь до директории проекта

if ! [[ -d ${basePath}/tmp ]]; then
    mkdir ${basePath}/tmp
fi

currentDir=${PWD}

cd ${basePath}/tmp
cmake ..
make
#ctest ..

cd ${currentDir}
