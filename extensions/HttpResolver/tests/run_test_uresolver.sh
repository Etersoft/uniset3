#!/bin/sh

HTTP_RESOLVER_PID=

function atexit()
{
	trap - EXIT

	[ -n "$HTTP_RESOLVER_PID" ] && kill $HTTP_RESOLVER_PID 2>/dev/null
	sleep 3
	[ -n "$HTTP_RESOLVER_PID" ] && kill -9 $HTTP_RESOLVER_PID 2>/dev/null

	exit $RET
}

trap atexit EXIT

. ./testsuite-functions.sh

init_testsuite || exit 1

../uniset3-httpresolver --confile uresolver-test-configure.xml &
HTTP_RESOLVER_PID=$!

sleep 5

./uniset3-start.sh -f ./run_test_uresolver $* -- --confile uresolver-test-configure.xml --lockDir ${ULOCKDIR} && RET=0 || RET=1

exit $RET
