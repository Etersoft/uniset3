#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..

. ./testsuite-functions.sh

init_testsuite || exit 1

./uniset3-start.sh -f ./tests-with-sm $* -- --confile uwebsocketgate-test-configure.xml --e-startup-pause 10 --lockDir ${ULOCKDIR} \
--ws-name UWebSocketGate1 --ws-httpserverhost-addr 127.0.0.1 --ws-httpserver-port 8081 --ws-max-cmd 3

#--ulog-add-levels system,repository --ws-log-add-levels any


