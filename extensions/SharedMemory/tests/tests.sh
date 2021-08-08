#!/bin/sh

. ./testsuite-functions.sh

init_testsuite || exit 1

./uniset3-start.sh -f ./tests $* -- --confile ./sm-configure.xml --pulsar-id Pulsar_S --pulsar-msec 1000 --e-filter evnt_test \
--heartbeat-node localhost --heartbeat-check-time 1000 --TestObject-startup-timeout 0 \
--uniset-object-size-message-queue 2000000 \
--lockDir ${ULOCKDIR} 

#--ulog-add-levels level2,warn,crit --TestObject-log-add-levels any
# repository,crit,warn --dlog-add-levels any
#--sm-log-add-levels any --ulog-add-levels level4,warn,crit \
#--TestObject-log-add-levels any
#--dlog-add-levels any
