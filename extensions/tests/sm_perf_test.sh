#!/bin/sh

. ./testsuite-functions.sh

init_testsuite || exit 1

#time -p ./uniset3-start.sh -vcall --dump-instr=yes --simulate-cache=yes --collect-jumps=yes ./sm_perf_test $* --confile sm_perf_test.xml --e-startup-pause 10
for i in `seq  1 5`; do
	./uniset3-start.sh -f ./sm_perf_test $* --confile sm_perf_test.xml --e-startup-pause 10 --lockDir ${ULOCKDIR} 2>>sm_perf_test.log
done
