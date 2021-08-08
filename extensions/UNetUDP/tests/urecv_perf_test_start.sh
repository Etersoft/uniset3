#!/bin/sh

. ./testsuite-functions.sh

init_testsuite || exit 1

./uniset3-start.sh -f ./urecv-perf-test $* -- --confile unetudp-test-configure.xml --lockDir ${ULOCKDIR}
