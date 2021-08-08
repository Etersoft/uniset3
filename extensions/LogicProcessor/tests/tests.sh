#!/bin/sh

. ./testsuite-functions.sh

init_testsuite || exit 1

./uniset3-start.sh -f ./tests $* -- --confile ./lp-configure.xml --sleepTime 50 --lockDir ${ULOCKDIR}
#--ulog-add-levels any
#--dlog-add-levels any
