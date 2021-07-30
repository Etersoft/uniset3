#!/bin/sh

./uniset3-start.sh -f ./uniset3-rrdserver --confile test.xml \
	--rrd-name RRDServer1 \
	--rrd-log-add-levels any $*
