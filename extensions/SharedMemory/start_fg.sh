#!/bin/sh

export LD_LIBRARY_PATH="../../lib/.libs;../lib/.libs"

ulimit -Sc 10000000000

./uniset3-start.sh -f ./uniset3-smemory --smemory-id SharedMemory  \
--confile test.xml --datfile test.xml --db-logging 1 --ulog-add-levels system,level1,level9 \
--sm-log-add-levels any $* --sm-run-logserver --activator-run-httpserver \

#--pulsar-id DO_C --pulsar-iotype DO --pulsar-msec 100

#--ulog-add-levels info,crit,warn,level9,system \
#--dlog-add-levels info,crit,warn \
