#!/bin/sh

./uniset3-start.sh -f ./uniset3-mqttpublisher --confile test.xml \
	--mqtt-name MQTTPublisher1 \
	--mqtt-filter-field mqtt \
	--mqtt-filter-value 1 \
	--mqtt-log-add-levels any $*
