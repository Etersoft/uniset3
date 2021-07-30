#!/bin/sh

uniset3-start.sh -f ./uniset3-backend-opentsdb --confile test.xml \
	--opentsdb-name BackendOpenTSDB \
	--opentsdb-log-add-levels any $*
