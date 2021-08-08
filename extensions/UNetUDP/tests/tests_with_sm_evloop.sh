#!/bin/sh

. ./testsuite-functions.sh

init_testsuite || exit 1

./uniset3-start.sh -f ./tests-with-sm $* -- --confile unetudp-test-configure.xml --e-startup-pause 10 \
--unet-name UNetExchange --unet-filter-field unet --unet-filter-value 1 --unet-maxdifferense 5 \
--unet-recv-timeout 1000 --unet-sendpause 500 --unet-update-strategy evloop --lockDir ${ULOCKDIR} 

#--unet-log-add-levels any --ulog-add-levels repostiory,crit,warn
