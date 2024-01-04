#!/bin/sh

. ./testsuite-functions.sh

init_testsuite || exit 1

./uniset3-start.sh -f ./test-folders-with-sm $* -- --confile opcua-server-test-folders-configure.xml --e-startup-pause 10 --lockDir ${ULOCKDIR} \
--opcua-filter-field iotype --smemory-id SharedMemory

# --opcua-log-add-levels any
