#!/bin/sh

. ./testsuite-functions.sh

init_testsuite || exit 1

echo "LOCKDIR: $ULOCKDIR"

# '--' - нужен для отделения аоргументов catch, от наших..
./uniset3-start.sh -f ./tests_with_conf $* -- --confile tests_with_conf.xml --prop-id2 -10 --lockDir ${ULOCKDIR}
