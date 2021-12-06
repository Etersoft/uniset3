#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..

. ./testsuite-functions.sh

init_testsuite || exit 1

export NO_RUN_REPOSITORY=1
./uniset3-start.sh -f ./tests-with-sm $* -- --confile uwebsocketgate-test-configure.xml --e-startup-pause 10 --lockDir ${ULOCKDIR} \
--ws-name UWebSocketGate1 --ws-host 127.0.0.1 --ws-port 8081 --ws-max-cmd 3 --grpc-host 127.0.0.1

#--ulog-add-levels any --ws-log-add-levels any --dlog-add-levels any
