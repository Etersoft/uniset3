############################################################################
# This file is part of the UniSet library                                  #
############################################################################

SUBDIRS=modbus

# install
devel_include_HEADERS = *.h *.tcc
devel_includedir = $(pkgincludedir)

all-local:
	test -L extensions || ln -s ../extensions/include extensions

update-json:
	wget -O $(top_builddir)/include/json.hpp https://github.com/nlohmann/json/raw/master/src/json.hpp

include $(top_builddir)/include.mk
