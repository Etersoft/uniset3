#!/bin/sh

./uniset3-start.sh -f "./uniset3-admin --confile test.xml --`basename $0 .sh` $1 $2 $3 $4 $5 $6"

exit $?
