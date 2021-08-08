#!/bin/sh

. ./testsuite-functions.sh

init_testsuite || exit 1

./uniset3-start.sh -f ./tests-with-sm $* -- --confile mbslave-test-configure.xml --e-startup-pause 10 \
--mbs-name MBSlave1 --mbs-type TCP --mbs-inet-addr 127.0.0.1 --mbs-inet-port 20048 \
--mbs-askcount-id SVU_AskCount_AS --mbs-respond-id RespondRTU_S --mbs-respond-invert 1 \
--mbs-filter-field mbs --mbs-filter-value 1 --mbs-initPause 100 --lockDir ${ULOCKDIR} 
#--mbs-log-add-levels any
