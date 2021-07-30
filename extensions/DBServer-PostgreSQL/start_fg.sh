#!/bin/sh

ulimit -Sc 1000000

./uniset3-start.sh -f ./uniset3-pgsql-dbserver --confile test.xml --name DBServer1 \
--pgsql-buffer-size 100 \
--pgsql-log-add-levels any $*
