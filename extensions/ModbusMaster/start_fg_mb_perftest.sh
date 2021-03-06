#!/bin/sh

MBPARAM=

for N in `seq 1 100`; do
    MBPARAM="$MBPARAM --mbtcp${N}-name MBTCP${N} --mbtcp${N}-confnode MBPerfTestMaster --mbtcp${N}-filter-field mbperf 
    --mbtcp${N}-filter-value $N --mbtcp${N}-persistent-connection 1 --mbtcp${N}-check-init-from-regmap --mbtcp${N}-log-add-levels warn,crit"
done

#echo "$MBPARAM"
#exit 0

./uniset3-start.sh -f ./mb-perf-test --dlog-add-levels crit,warn ${MBPARAM} \
--confile test.xml \
--smemory-id SharedMemory \
$*
