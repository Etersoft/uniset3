#!/bin/sh

. ./testsuite-functions.sh

init_testsuite || exit 1

./uniset3-start.sh -f ./run_test_mbtcpmultimaster $* -- --confile mbmaster-test-configure.xml --e-startup-pause 10 \
--lockDir ${ULOCKDIR} \
--mbtcp-name MBTCPMultiMaster1 \
--smemory-id SharedMemory \
--mbtcp-filter-field mb \
--mbtcp-filter-value 1 \
--mbtcp-polltime 50 --mbtcp-recv-timeout 500 --mbtcp-checktime 1000 --mbtcp-timeout 3000 --mbtcp-ignore-timeout 3000 \
--dlog-add-levels warn,crit
# --mbtcp-log-add-levels any
# --dlog-add-levels any
#--mbtcp-force-out 1
#--dlog-add-levels any
