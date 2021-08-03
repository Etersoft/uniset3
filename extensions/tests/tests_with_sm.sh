#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
#cd ../../Utilities/Admin/
#./create_links.sh
#./uniset3-start.sh -f ./create
#./uniset3-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
#cd -

[ -d "/tmp/uniset3-testsuite.lock" ] && rm -f /tmp/uniset3-testsuite.lock/*
mkdir -p /tmp/uniset3-testsuite.lock

./uniset3-start.sh -f ./tests_with_sm $* -- --confile tests_with_sm.xml --e-startup-pause 10 --ulog-add-levels any
