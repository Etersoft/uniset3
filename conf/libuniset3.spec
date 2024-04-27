%def_enable docs
%def_enable mysql
%def_enable sqlite
%def_enable pgsql
%def_enable python
%def_enable io
%def_enable logicproc
#%def_enable modbus
%def_disable tests
%def_disable mqtt
%def_disable netdata
%def_enable api
%def_enable logdb
%def_enable opentsdb
%def_enable uwebsocket
%def_enable clickhouse
%def_enable opcua

%ifarch %ix86
%def_enable com485f
%else
%def_disable com485f
%endif

%define oname uniset3

Name: libuniset3
Version: 3.0.0
Release: alt0.1
Summary: UniSet - library for building distributed industrial control systems

License: LGPL-2.1
Group: Development/C++
Url: http://wiki.etersoft.ru/UniSet

Packager: Pavel Vainerman <pv@altlinux.ru>

# Git: http://git.etersoft.ru/projects/asu/uniset.git
Source: %name-%version.tar

# Automatically added by buildreq on Thu Aug 12 2021
# optimized out: fontconfig fonts-ttf-liberation-narrow glibc-kernheaders-generic glibc-kernheaders-x86 libabseil-cpp-devel libcares-devel libcrypt-devel libgpg-error libgrpc++-devel libpoco-net libprotobuf-devel libre2-devel libsasl2-3 libsqlite3-devel libssl-devel libstdc++-devel perl pkg-config protobuf-compiler python-modules python-modules-compiler python-modules-distutils python2-base python3 python3-base python3-module-paste sh4 tzdata zlib-devel
BuildRequires: catch-devel gcc-c++ grpc-plugins libev-devel libgrpc-devel libpoco-devel libsigc++2-devel libxml2-devel xsltproc

# for uniset2-codegen
BuildPreReq: xsltproc

# due -std=c++11 using
# BuildPreReq: gcc5 >= 4.8
# Must be gcc >= 4.7

%if_enabled io
BuildRequires: libcomedi-devel
%endif

%if_enabled mysql
# build with mariadb 
BuildRequires: libmariadb-devel
%endif

%if_enabled sqlite
BuildRequires: libsqlite3-devel
%endif

%if_enabled pgsql
BuildRequires: libpqxx-devel >= 7.6.0
%endif

%if_enabled clickhouse
BuildRequires: libclickhouse-cpp-devel >= 2.4.0
%endif

%if_enabled mqtt
BuildRequires: libmosquitto-devel
%endif

%if_enabled opcua
BuildRequires: libopen62541-devel libopen62541pp-devel >= 0.13.0-alt1
%endif


%if_enabled netdata
BuildRequires: netdata
%endif

%if_enabled python
BuildRequires: python3-dev
BuildRequires(pre): rpm-build-python3

# swig
# add_findprov_lib_path %python_sitelibdir/%oname
%endif

%if_enabled docs
BuildRequires: doxygen graphviz ImageMagick-tools
%endif

#set_verify_elf_method textrel=strict,rpath=strict,unresolved=strict

%description
UniSet is a library for distributed control systems.
There are set of base components to construct this kind of systems:
* base interfaces for your implementation of control algorithms.
* algorithms for the discrete and analog input/output based on COMEDI interface.
* IPC mechanism based on CORBA (omniORB).
* logging system based on MySQL,SQLite,PostgreSQL databases.
* Web interface to display logging and statistic information.
* utilities for system's configuration based on XML.

UniSet have been written in C++ and IDL languages but you can use another languages in your
add-on components. The main principle of the UniSet library's design is a maximum integration
with open source third-party libraries. UniSet provide the consistent interface for all
add-on components and third-party libraries.

More information in Russian:

%package devel
Group: Development/C
Summary: Libraries needed to develop for UniSet
Requires: %name = %version-%release

%description devel
Libraries needed to develop for UniSet.


%if_enabled python
%package -n python3-module-%oname
Group: Development/Python
Summary: python interface for libuniset
Requires: %name = %version-%release

# py_provides UGlobal UInterface UniXML uniset

%description -n python3-module-%oname
Python interface for %name
%endif

%if_enabled netdata
%package netdata-plugin
Group: Development/Tools
Summary: python plugin for netdata
Requires: python-module-%oname

%description netdata-plugin
python plugin for netdata
%endif

%package utils
Summary: UniSet utilities
Group: Development/Tools
Requires: %name = %version-%release
Provides: %oname-utils
Obsoletes: %oname-utils

%description utils
UniSet utilities

%if_enabled docs

%package docs
Group: Development/C++
Summary: Documentations for developing with UniSet
# Requires: %name = %version-%release
BuildArch: noarch

%description docs
Documentations for developing with UniSet
%endif

%package extension-common
Group: Development/C++
Summary: libUniSet3 extensions common
Requires: %name = %version-%release

%description extension-common
Extensions for libuniset

%package extension-common-devel
Group: Development/C++
Summary: Libraries needed to develop for uniset extensions
Requires: %name-devel = %version-%release
Provides: %name-extentions-devel
Obsoletes: %name-extentions-devel

%description extension-common-devel
Libraries needed to develop for uniset extensions

%if_enabled api
%package extension-wsgate
Group: Development/Tools
Summary: Websocket gate for uniset

%description extension-wsgate
Websocket gate for uniset

%package extension-wsgate-devel
Group: Development/Tools
Summary: Websocket gate develop libraries

%description extension-wsgate-devel
Websocket gate develop libraries
%endif

%if_enabled mysql
%package extension-mysql
Group: Development/Databases
Summary: MySQL-dbserver implementatioin for UniSet
Requires: %name-extension-common = %version-%release

%description extension-mysql
MySQL dbserver for %name

%package extension-mysql-devel
Group: Development/Databases
Summary: Libraries needed to develop for uniset MySQL
Requires: %name-extension-common-devel = %version-%release

%description extension-mysql-devel
Libraries needed to develop for uniset MySQL
%endif

%if_enabled sqlite
%package extension-sqlite
Group: Development/Databases
Summary: SQLite-dbserver implementatioin for UniSet
Requires: %name-extension-common = %version-%release

%description extension-sqlite
SQLite dbserver for %name

%package extension-sqlite-devel
Group: Development/Databases
Summary: Libraries needed to develop for uniset SQLite
Requires: %name-extension-common = %version-%release

%description extension-sqlite-devel
Libraries needed to develop for uniset SQLite

%if_enabled logdb
%package extension-logdb
Group: Development/C++
Summary: database for %name logs (sqlite)
Requires: %name-extension-sqlite = %version-%release

%description extension-logdb
Database (sqlite) for logs for %name
%endif
%endif

%if_enabled opentsdb
%package extension-opentsdb
Group: Development/C++
Summary: backend for OpenTSDB
Requires: %name-extension-common = %version-%release

%description extension-opentsdb
Backend for OpenTSDB

%package extension-opentsdb-devel
Group: Development/Databases
Summary: Libraries needed to develop for uniset OpenTSDB backend
Requires: %name-extension-common-devel = %version-%release

%description extension-opentsdb-devel
Libraries needed to develop for backend for OpenTSDB

%endif

%if_enabled clickhouse
%package extension-clickhouse
Group: Development/C++
Summary: backend for ClickHouse
Requires: %name-extension-common = %version-%release

%description extension-clickhouse
Backend for ClickHouse

%package extension-clickhouse-devel
Group: Development/Databases
Summary: Libraries needed to develop for uniset ClickHouse backend
Requires: %name-extension-common-devel = %version-%release

%description extension-clickhouse-devel
Libraries needed to develop for backend for ClickHouse

%endif

%if_enabled pgsql
%package extension-pgsql
Group: Development/Databases
Summary: PostgreSQL-dbserver implementatioin for UniSet
Requires: %name-extension-common = %version-%release

%description extension-pgsql
PostgreSQL dbserver for %name

%package extension-pgsql-devel
Group: Development/Databases
Summary: Libraries needed to develop for uniset PostgreSQL
Requires: %name-extension-common-devel = %version-%release

%description extension-pgsql-devel
Libraries needed to develop for uniset PostgreSQL
%endif

%if_enabled logicproc
%package extension-logicproc
Group: Development/C++
Summary: LogicProcessor extension for libUniSet
Requires: %name-extension-common = %version-%release

%description extension-logicproc
LogicProcessor for %name

%package extension-logicproc-devel
Group: Development/C++
Summary: Libraries needed to develop for uniset LogicProcesor extension
Requires: %name-extension-common-devel = %version-%release

%description extension-logicproc-devel
Libraries needed to develop for uniset LogicProcessor extension
%endif

%if_enabled io
%package extension-io
Group: Development/C++
Summary: IOControl with io for UniSet
Requires: %name-extension-common = %version-%release

%description extension-io
IOControl for %name

%package extension-io-devel
Group: Development/C++
Summary: Libraries needed to develop for uniset IOControl (io)
Requires: %name-extension-common-devel = %version-%release

%description extension-io-devel
Libraries needed to develop for uniset IOControl (io)
%endif

%if_enabled mqtt
%package extension-mqtt
Group: Development/C++
Summary: MQTTpublisher from UniSet
Requires: %name-extension-common = %version-%release

%description extension-mqtt
MQTT for %name

%package extension-mqtt-devel
Group: Development/C++
Summary: Libraries needed to develop for uniset MQTT extension
Requires: %name-extension-common-devel = %version-%release

%description extension-mqtt-devel
Libraries needed to develop for uniset MQTT extension
%endif

%if_enabled opcua
%package extension-opcua
Group: Development/C++
Summary: OPC UA support for %{name}
Requires: %name-extension-common = %version-%release

%description extension-opcua
OPC UA support for %{name}

%package extension-opcua-devel
Group: Development/C++
Summary: Libraries needed to develop for uniset OPC UA extension
Requires: %name-extension-common-devel = %version-%release

%description extension-opcua-devel
Libraries needed to develop for uniset OPC UA extension
%endif

%if_enabled api
%package extension-api-gateway
Group: Development/C++
Summary: HTTP API Gateway for %name
Requires: %name-extension-common = %version-%release

%description extension-api-gateway
HTTP API Gateway for %name

%package extension-api-gateway-devel
Group: Development/C++
Summary: Libraries needed to develop for uniset HTTP API Gateway extension
Requires: %name-extension-common-devel = %version-%release

%description extension-api-gateway-devel
Libraries needed to develop for uniset HTTP API Gateway extension
%endif

%prep
%setup

%build
%autoreconf
%if "%__gcc_version_major" < "12"
%add_optflags -std=c++17
%endif
%configure %{subst_enable docs} %{subst_enable mysql} %{subst_enable sqlite} %{subst_enable pgsql} %{subst_enable python} %{subst_enable io} %{subst_enable logicproc} %{subst_enable tests} %{subst_enable mqtt} %{subst_enable api} %{subst_enable netdata} %{subst_enable logdb} %{subst_enable com485f} %{subst_enable opentsdb} %{subst_enable clickhouse} %{subst_enable uwebsocket} %{subst_enable opcua}
%make_build

%install
%makeinstall_std
rm -f %buildroot%_libdir/*.la

%if_enabled docs
rm -f %buildroot%_docdir/%oname/html/*.map
rm -f %buildroot%_docdir/%oname/html/*.md5
%endif

%files utils
%_bindir/%oname-admin
%_bindir/%oname-mb*
%_bindir/%oname-nullController
%_bindir/%oname-sviewer-text
%_bindir/%oname-smonit
%_bindir/%oname-vmonit
%_bindir/%oname-simitator
%_bindir/%oname-log
%_bindir/%oname-logserver-wrap
%_bindir/%oname-start*
%_bindir/%oname-stop*
%_bindir/%oname-func*
%_bindir/%oname-codegen
%_bindir/%oname-log2val
%_bindir/%oname-urepository
%dir %_datadir/%oname/
%dir %_datadir/%oname/xslt/
%_datadir/%oname/xslt/*.xsl
%_datadir/%oname/xslt/skel*


%files
%_libdir/libUniSet3.so.*

%files devel
%dir %_includedir/%oname/
%_includedir/%oname/*.h
%_includedir/%oname/*.proto
%_includedir/%oname/*.tcc
%_includedir/%oname/modbus/

%_libdir/libUniSet3.so
%_pkgconfigdir/libUniSet3.pc

%if_enabled mysql
%files extension-mysql
%_bindir/%oname-mysql-*dbserver
%_libdir/*-mysql.so*

%files extension-mysql-devel
%_pkgconfigdir/libUniSet3MySQL.pc
%_includedir/%oname/extensions/mysql/
%endif

%if_enabled sqlite
%files extension-sqlite
%_bindir/%oname-sqlite-*dbserver
%_libdir/*-sqlite.so.*

%files extension-sqlite-devel
%_pkgconfigdir/libUniSet3SQLite.pc
%_includedir/%oname/extensions/sqlite/
%_libdir/*-sqlite.so

%if_enabled logdb
%files extension-logdb
%_bindir/%oname-logdb*
%endif
%endif

%if_enabled opentsdb
%files extension-opentsdb
%_bindir/%oname-backend-opentsdb*
%_libdir/libUniSet3BackendOpenTSDB.so.*

%files extension-opentsdb-devel
%_pkgconfigdir/libUniSet3BackendOpenTSDB.pc
%_includedir/%oname/extensions/BackendOpenTSDB.h
%_libdir/libUniSet3BackendOpenTSDB.so
%endif

%if_enabled clickhouse
%files extension-clickhouse
%_bindir/%oname-backend-clickhouse*
%_bindir/%oname-clickhouse-admin
%_bindir/%oname-clickhouse-helper
%_libdir/libUniSet3BackendClickHouse.so.*
%_datadir/%oname/clickhouse/
%dir %_datadir/%oname/clickhouse/init.d

%files extension-clickhouse-devel
%_pkgconfigdir/libUniSet3BackendClickHouse.pc
%_includedir/%oname/extensions/BackendClickHouse.h
%_libdir/libUniSet3BackendClickHouse.so
%endif

%if_enabled pgsql
%files extension-pgsql
%_bindir/%oname-pgsql-*dbserver
%_libdir/*-pgsql.so.*

%files extension-pgsql-devel
%_pkgconfigdir/libUniSet3PostgreSQL.pc
%_includedir/%oname/extensions/pgsql/
%_libdir/*-pgsql.so
%endif

%if_enabled python
%files -n python3-module-%oname
%python3_sitelibdir/*
%python3_sitelibdir_noarch/%oname/*
%dir %python3_sitelibdir_noarch/%oname
%endif

%if_enabled netdata
%files netdata-plugin
%_libdir/netdata/python.d/*.*
%config(noreplace) %_sysconfdir/netdata/python.d/*.conf
%endif

%if_enabled docs
%files docs
%_docdir/%oname/
%endif

%files extension-common
%_bindir/%oname-mtr-conv
%_bindir/%oname-mtr-setup
%_bindir/%oname-mtr-read
%_bindir/%oname-vtconv
%_bindir/%oname-rtuexchange
%_bindir/%oname-smemory
%_bindir/%oname-smviewer
%_bindir/%oname-unet*

%_libdir/libUniSet3Extensions.so.*
%_libdir/libUniSet3MB*.so.*
%_libdir/libUniSet3RT*.so.*
%_libdir/libUniSet3Shared*.so.*
%_libdir/libUniSet3UNetUDP*.so.*

%if_enabled logicproc
%files extension-logicproc
%_libdir/libUniSet3LP*.so.*
%_bindir/%oname-logicproc
%_bindir/%oname-plogicproc

%files extension-logicproc-devel
%_pkgconfigdir/libUniSet3Log*.pc
%_libdir/libUniSet3LP*.so
%_includedir/%oname/extensions/logicproc/
%endif

%if_enabled io
%files extension-io
%_bindir/%oname-iocontrol
%_bindir/%oname-iotest
%_bindir/%oname-iocalibr
%_libdir/libUniSet3IO*.so.*

%files extension-io-devel
%_libdir/libUniSet3IO*.so
%_pkgconfigdir/libUniSet3IO*.pc
%_includedir/%oname/extensions/io/
%endif

%if_enabled mqtt
%files extension-mqtt
%_bindir/%oname-mqtt*
%_libdir/libUniSet3MQTTPublisher*.so.*

%files extension-mqtt-devel
%_pkgconfigdir/libUniSet3MQTTPublisher*.pc
%_libdir/libUniSet3MQTTPublisher*.so
%_includedir/%oname/extensions/mqtt/
%endif

%if_enabled opcua
%files extension-opcua
%_bindir/%oname-opcua*
%_libdir/libUniSet3OPCUA*.so.*

%files extension-opcua-devel
%_pkgconfigdir/libUniSet3OPCUA*.pc
%_libdir/libUniSet3OPCUA*.so
%_includedir/%oname/extensions/opcua/
%endif

%if_enabled api
%if_enabled uwebsocket
%files extension-wsgate
%_bindir/%oname-wsgate*
%_libdir/libUniSet3UWebSocketGate*.so.*

%files extension-wsgate-devel
%_pkgconfigdir/libUniSet3UWebSocketGate*.pc
%_libdir/libUniSet3UWebSocketGate*.so
%_includedir/%oname/extensions/wsgate/
%endif
%endif

%files extension-common-devel
%dir %_includedir/%oname/extensions
%_includedir/%oname/extensions/*.*
%if_enabled opentsdb
%exclude %_includedir/%oname/extensions/BackendOpenTSDB.h
%endif
%if_enabled clickhouse
%exclude %_includedir/%oname/extensions/BackendClickHouse.h
%endif
%_libdir/libUniSet3Extensions.so
%_libdir/libUniSet3MB*.so
%_libdir/libUniSet3RT*.so
%_libdir/libUniSet3Shared*.so
%_libdir/libUniSet3UNetUDP.so
%_pkgconfigdir/libUniSet3Extensions.pc
%_pkgconfigdir/libUniSet3MB*.pc
%_pkgconfigdir/libUniSet3RT*.pc
%_pkgconfigdir/libUniSet3Shared*.pc
%_pkgconfigdir/libUniSet3UNet*.pc

#%_pkgconfigdir/libUniSet3*.pc
%exclude %_pkgconfigdir/libUniSet3.pc

%if_enabled api
%files extension-api-gateway
%_bindir/%oname-api-gateway*
%_libdir/libUniSet3HttpAPIGateway*.so.*

%files extension-api-gateway-devel
%_pkgconfigdir/libUniSet3HttpAPIGateway*.pc
%_libdir/libUniSet3HttpAPIGateway*.so
%_includedir/%oname/extensions/apigateway/
%endif


# history of current unpublished changes

%changelog
* Thu Jul 29 2021 Pavel Vainerman <pv@altlinux.ru> 2.14.1-alt1
- [uwebsocket]: refactoring (update help, rename parameters)
