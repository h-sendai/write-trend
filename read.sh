#!/bin/sh

./read-trend -i 0.1 test.file   > /dev/shm/read.log.1 &
./read-trend -i 0.1 test.file.2 > /dev/shm/read.log.2 &
