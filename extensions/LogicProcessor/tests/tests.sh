#!/bin/sh

./uniset3-start.sh -f ./uniset3-admin --confile ./lp-configure.xml --create
./uniset3-start.sh -f ./uniset3-admin --confile ./lp-configure.xml --exist | grep -q UNISET_LP/Controllers || exit 1

./uniset3-start.sh -f ./tests $* -- --confile ./lp-configure.xml --sleepTime 50
#--ulog-add-levels any
#--dlog-add-levels any
