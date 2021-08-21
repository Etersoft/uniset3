#!/bin/sh

export LD_LIBRARY_PATH="../../lib/.libs;../../lib/pyUniSet/.libs"

./uniset3-start.sh -f ./testUC.py --ulog-add-levels info,warn,crit
