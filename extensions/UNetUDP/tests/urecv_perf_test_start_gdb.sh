#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../../Utilities/Admin/
./uniset3-start.sh -f ./create_links.sh
./uniset3-start.sh -f ./create

./uniset3-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -

./uniset3-start.sh -g ./urecv-perf-test $* -- --confile unetudp-test-configure.xml
