#!/bin/sh

ulimit -Sc 1000000

#for i in `seq 1 20`; 
#do
	uniset3-start.sh -f ./uniset3-simitator --confile test.xml --sid $*
#done

#wait

#--ulog-add-levels info,crit,warn,level9,system

