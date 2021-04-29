#!/bin/bash

DIR="$( cd "$( dirname "$0"  )" && pwd  )"
echo ${DIR}

echo "... star ..."
cmake .. -DARM=ON && make
if [ $? -ne 0 ]; then
    echo "make failed"
else
    echo "make succeed"
fi

cd .. && mkdir -p delivery && cd delivery && tar -czvf server_arm.tar.gz ../bin/server_arm
if [ $? -ne 0 ]; then
    echo "pack failed"
else
    echo "pack succeed"
fi

echo "... end ..."