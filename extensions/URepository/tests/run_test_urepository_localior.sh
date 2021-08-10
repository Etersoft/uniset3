#!/bin/sh

UREPOSITORY_PID=

. ./testsuite-functions.sh

function local_exit()
{
    [ -n "$UREPOSITORY_PID" ] && kill $UREPOSITORY_PID 2>/dev/null
    sleep 3
    [ -n "$UREPOSITORY_PID" ] && kill -9 $UREPOSITORY_PID 2>/dev/null
}

init_testsuite || exit 1


../uniset3-urepository --confile urepository-test-configure-localior.xml --lockDir ${ULOCKDIR} &

UREPOSITORY_PID=$!

sleep 5

./uniset3-start.sh -f ./run_test_urepository $* -- --confile urepository-test-configure-localior.xml --lockDir ${ULOCKDIR} && RET=0 || RET=1

exit $RET
