#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..

. ./testsuite-functions.sh

init_testsuite || exit 1

MAX=200

PARAMS=
for n in `seq 1 $MAX`; do
   PARAMS="${PARAMS} --mbs${n}-name MBTCP${n} --mbs${n}-inet-port ${n+80000} --mbs${n}-inet-addr 127.0.0.1 \
   --mbs${n}-type TCP --mbs${n}-filter-field mbtcp --mbs${n}-filter-value 2 --mbs${n}-confnode MBSlave2 \
   --mbs${n}-log-add-levels crit,warn"
done

./uniset3-start.sh -f ./mbslave-perf-test --confile test.xml ${PARAMS} --numproc $MAX --lockDir ${ULOCKDIR} $*
