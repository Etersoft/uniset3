#!/bin/sh

ulimit -Sc 1000000

uniset-start.sh -f ./uniset-mysql-dbserver --confile test.xml --name DBServer --unideb-add-levels info,crit,warn,level9,system