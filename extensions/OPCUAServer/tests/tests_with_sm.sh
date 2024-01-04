#!/bin/sh

. ./testsuite-functions.sh

init_testsuite || exit 1

./uniset3-start.sh -f ./tests-with-sm $* -- --confile opcua-server-test-configure.xml --e-startup-pause 10 --lockDir ${ULOCKDIR} \
--opcua-filter-field iotype --smemory-id SharedMemory

# --opcua-log-add-levels any 
