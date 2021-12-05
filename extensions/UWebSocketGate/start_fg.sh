#!/bin/sh

ulimit -Sc 1000000

./uniset3-start.sh -f ./uniset3-wsgate --confile test.xml --ws-name UWebSocketGate1 --ws-log-add-levels any --ws-log-verbosity 5 $*
