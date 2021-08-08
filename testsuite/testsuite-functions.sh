#!/bin/sh

ULOCKDIR=

function create_lock_dir()
{
    ULOCKDIR=$(mktemp -d /tmp/utest-XXXXXXX)/
    mkdir -p ${ULOCKDIR}
}

atexit()
{
    local rc=$?
    trap - EXIT

    [ -d "$ULOCKDIR" ] && rm -rf $ULOCKDIR

    exit $rc
}

function init_testsuite()
{
    trap '' HUP INT QUIT PIPE TERM
    trap atexit EXIT
    create_lock_dir
}
