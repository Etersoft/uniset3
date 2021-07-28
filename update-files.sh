#!/bin/sh

find ./ -type f -name '*.cc' -or -name '*.cpp' -or -name '*.h' -or -name '*.src.xml' -or -name '*.am' | grep -v '_SK' | sort -n > uniset3.files
echo ./conf/configure.xml >> uniset3.files
