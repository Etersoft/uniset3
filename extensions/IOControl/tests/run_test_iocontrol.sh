#!/bin/sh

. ./testsuite-functions.sh

init_testsuite || exit 1

./uniset3-start.sh -f ./run_test_iocontrol $* -- --confile iocontrol-test-configure.xml --e-startup-pause 10 \
--lockDir ${ULOCKDIR} \
--io-name IOControl1 \
--smemory-id SharedMemory \
--io-s-filter-field io \
--io-s-filter-value 1
#--io-log-add-levels any
