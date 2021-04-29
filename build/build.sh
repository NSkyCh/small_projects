#!/bin/bash

DIR="$( cd "$( dirname "$0"  )" && pwd  )"
echo ${DIR}

echo "... star ..."
cmake .. && make
if [ $? -ne 0 ]; then
    echo "make failed"
else
    echo "make succeed"
fi

cd .. && mkdir -p delivery && cd delivery && tar -czvf server.tar.gz -C ../bin/*

echo "... end ..."