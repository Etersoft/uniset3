#!/bin/sh

./uniset3-start.sh -f ./uniset3-plogicproc --schema schema.xml \
--smemory-id SharedMemory --name LProcessor \
--confile test.xml --ulog-add-levels info,crit,warn,level9,system

