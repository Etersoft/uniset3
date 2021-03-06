#!/bin/sh

ulimit -Sc 1000000

#uniset3-start.sh -g \
./uniset3-logdb --confile test.xml --logdb-name LogDB \
 --logdb-log-add-levels any \
 --logdb-dbfile ./test.db \
 --logdb-db-buffer-size 5 \
 --logdb-httpserver-port 8888 \
 --logdb-db-max-records 20000 \
 $*

