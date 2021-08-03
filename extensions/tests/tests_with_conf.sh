#!/bin/sh

[ -d "/tmp/uniset3-testsuite.lock" ] && rm -f /tmp/uniset3-testsuite.lock/*
mkdir -p /tmp/uniset3-testsuite.lock

# '--' - нужен для отделения аоргументов catch, от наших..
./uniset3-start.sh -f ./tests_with_conf $* -- --confile tests_with_conf.xml --prop-id2 -10
