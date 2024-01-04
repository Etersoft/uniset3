#!/bin/sh

. ./testsuite-functions.sh

init_testsuite || exit 1

./uniset3-start.sh -f ./run_test_opcua_exchange $* -- --confile opcua-exchange-test-configure.xml --e-startup-pause 10 \
--lockDir ${ULOCKDIR} \
--opcua-name OPCUAExchange1 \
--smemory-id SharedMemory \
--opcua-filter-field opc \
--opcua-filter-value 1 
#--opcua-log-add-levels level5,-level6,-level7
