#!/bin/sh

ulimit -Sc 1000000

./uniset3-start.sh -f ./uniset3-sqlite-dbserver --confile test.xml --name DBServer1 \
--sqlite-log-add-levels any \
--ulog-add-levels info,crit,warn,level9,system \
--sqlite-buffer-size 100

