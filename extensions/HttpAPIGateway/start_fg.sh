#!/bin/sh

ulimit -Sc 1000000

./uniset3-start.sh -f ./uniset3-api-gateway --confile test.xml --api-name HttpAPIGateway1 --api-log-add-levels any --api-log-verbosity 5 $*
