#!/bin/sh

HTTP_API_PID=

. ./testsuite-functions.sh

function local_exit()
{
    [ -n "$HTTP_API_PID" ] && kill $HTTP_API_PID 2>/dev/null
    sleep 3
    [ -n "$HTTP_API_PID" ] && kill -9 $HTTP_API_PID 2>/dev/null
}

init_testsuite || exit 1


../uniset3-api-gateway --confile apigateway-test-configure.xml --lockDir ${ULOCKDIR} &
# --api-log-add-levels any &

HTTP_API_PID=$!

sleep 5

./uniset3-start.sh -f ./run_test_apigateway $* -- --confile apigateway-test-configure.xml --lockDir ${ULOCKDIR} && RET=0 || RET=1

exit $RET
