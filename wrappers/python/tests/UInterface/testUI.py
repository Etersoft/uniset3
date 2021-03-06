#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys

sys.path.append('../../')
sys.path.append('../../lib/lib/')
sys.path.append('../../lib/lib/.libs')

from UniSet import *

if __name__ == "__main__":

    lst = Params_inst()

    for i in range(0, len(sys.argv)):
        if i >= Params.max:
            break;
        lst.add(sys.argv[i])

    try:
        uniset_init_params(lst, "test.xml");

        obj1 = UProxyObject("TestProc")
        obj1.addToAsk(1)
        obj1.addToAsk(10)

#        obj2 = UProxyObject("TestProc1")

        uniset_activate_objects()

        obj1.askSensor(10)

        while True:
            print("sleep...")
            print("getValue: %d=%d" % (10, obj1.getValue(10)))
            time.sleep(2)

        print("getShortName: id=%d name=%s" % (1, getShortName(1)))
        print("     getName: id=%d name=%s" % (1, getName(1)))
        print(" getTextName: id=%d name=%s" % (1, getTextName(1)))
        print("\n")
        print("getShortName: id=%d name=%s" % (2, getShortName(2)))
        print("     getName: id=%d name=%s" % (2, getName(2)))
        print(" getTextName: id=%d name=%s" % (2, getTextName(2)))
        print(" getObjectID: id=%d name=%s" % (getObjectID("TestProc"), "TestProc"))

        try:
            print("getValue: %d=%d" % (1, getValue(1)))
        except UException as e:
            print("getValue exception: " + str(e.getError()))

        try:
            print("setValue: %d=%d" % (14, 22))
            setValue(14, 22)
        except UException as e:
            print("setValue exception: " + str(e.getError()))

    except UException as e:
        print("(testUI): catch exception: " + str(e.getError()))
