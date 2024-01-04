#!/bin/sh

./uniset3-start.sh -f ./uniset3-rtuexchange --confile test.xml \
	--smemory-id SharedMemory \
	--rs-dev /dev/cbsideA0 \
	--rs-name RTUExchange \
	--rs-speed 115200 \
	--rs-filter-field rs \
	--rs-filter-value 4 \
	--rs-log-add-levels info,crit,warn,level4,level3 \
	--rs-force 0 \
	--rs-force-out 0 \
	--rs-polltime 500 \
	--rs-set-prop-prefix \

#,level3
#	--rs-force 1 \
