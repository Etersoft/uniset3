#!/bin/sh

gdb --batch -n -ex "thread apply all bt" $1 $2
# | ssh xxxxxx