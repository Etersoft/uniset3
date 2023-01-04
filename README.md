UniSet ![Gihub testsuite Status](https://github.com/Etersoft/uniset3/actions/workflows/testsuite.yml/badge.svg)
======

UniSet is a library for distributed control systems development.
There are set of base components to construct this kind of systems:
* base interfaces for your implementation of control algorithms.
* algorithms for a discrete and analog input/output based on [COMEDI](https://github.com/Linux-Comedi/comedi) interface.
* IPC mechanism based on GRPC (protobuf).
* logging system based on MySQL, SQLite, PostgreSQL databases.
* logging to TSDB (influxdb, opentsdb)
* supported MQTT ([libmosquittopp](http://mosquitto.org))
* fast network protocol based on udp (UNet)
* utilities for system's configuration based on XML.
* python interface
* go interface (experimental)
* API Gateway
* Websockets

UniSet have been written in C++ and IDL languages but you can use another languages in your
add-on components. The main principle of the UniSet library's design is a maximum integration
with open source third-party libraries. UniSet provides the consistent interface for all
add-on components and third-party libraries. Python wrapper helps in using the library
in python scripts.

libuniset requires minimum C++11

UniSet3 based on [UniSet2](https://github.com/Etersoft/uniset2)

periodically checked by [PVS-Studio Analyzer](https://www.viva64.com/en/pvs-studio/)
