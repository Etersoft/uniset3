#!/bin/sh

ulimit -Sc 1000000

./uniset3-start.sh -f ./uniset3-nullController --name SharedMemory1 --confile test.xml --ulog-add-levels any $*
#info,warn,crit,system,level9 > 1.log
#--c-filter-field cfilter --c-filter-value test1 --s-filter-field io --s-filter-value 1

