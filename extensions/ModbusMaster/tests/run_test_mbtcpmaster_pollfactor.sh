#!/bin/sh

. ./testsuite-functions.sh

init_testsuite || exit 1

./uniset3-start.sh -f ./run_test_mbtcpmaster_pollfactor $* -- --confile mbmaster-pollfactor-test-configure.xml --e-startup-pause 10 \
--lockDir ${ULOCKDIR} \
--mbtcp-name MBTCPMaster1 \
--smemory-id SharedMemory \
--mbtcp-filter-field mb \
--mbtcp-filter-value 1 \
--mbtcp-gateway-iaddr localhost \
--mbtcp-gateway-port 20048 \
--mbtcp-polltime 50 --mbtcp-recv-timeout 500 --mbtcp-timeout 2500

#--mbtcp-log-add-levels any,warn,crit 
#--ulog-add-levels any

#--mbtcp-default-mbinit-ok 1

#--mbtcp-force-out 1
#--dlog-add-levels any
