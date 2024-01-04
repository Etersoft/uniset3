#!/bin/sh

. ./testsuite-functions.sh

init_testsuite || exit 1

./uniset3-start.sh -f ./test-method-with-sm $* -- --confile opcua-server-test-method-configure.xml --e-startup-pause 10 --lockDir ${ULOCKDIR} \
--opcua-filter-field iotype --smemory-id SharedMemory 

# --opcua-log-add-levels any
