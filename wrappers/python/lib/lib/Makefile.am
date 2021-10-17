if ENABLE_PYTHON

SWIG=swig

#python_SCRIPTS = pyUniSet.py pyUConnector.py pyUModbus.py pyUExceptions.py
pydir=$(pkgpythondir)/lib
py_DATA = __init__.py UniSet.py UConnector.py UModbus.py UExceptions.py

BUILD_SRCDIR=$(srcdir)/../../../core
BUILD_SRC=UModbus.cc UConnector.cc UProxyObject.cc

UniSet.py: UInterface_wrap.cxx _UniSet3.la

UConnector.py: UConnector_wrap.cxx _UConnector3.la

UModbus.py: _UModbus3.la UModbus_wrap.cxx

UExceptions.py: _UExceptions3.la


pyexec_LTLIBRARIES = _UConnector3.la _UModbus3.la _UExceptions3.la _UniSet3.la

_UniSet3_la_SOURCES 	= PyUInterface.cc UInterface_wrap.cxx UProxyObject.cc
_UniSet3_la_CXXFLAGS	= -I$(BUILD_SRCDIR) $(UNISET_CFLAGS) $(UNISET_EXT_CFLAGS) $(PYTHON_CFLAGS)
_UniSet3_la_LDFLAGS 	= -module -avoid-version
_UniSet3_la_LIBADD 	= $(UNISET_LIBS) $(UNISET_EXT_LIBS) $(PYTHON_LIBS)

UInterface_wrap.cxx: UInterface.i UProxyObject.i PyUInterface.h $(BUILD_SRCDIR)/UProxyObject.h
	$(SWIG) -python -py3 -c++ -outcurrentdir -I$(BUILD_SRCDIR) UInterface.i

#PyUObject_wrap.cxx: UObject.i PyUObject.h
#	swig -python -c++ -classic UObject.i

_UConnector3_la_SOURCES 	= UConnector.cc UConnector_wrap.cxx
_UConnector3_la_CXXFLAGS	= -I$(BUILD_SRCDIR) $(UNISET_CFLAGS) $(PYTHON_CFLAGS)
_UConnector3_la_LDFLAGS 	= -module -avoid-version
_UConnector3_la_LIBADD 	= $(UNISET_LIBS) $(PYTHON_LIBS)

UConnector_wrap.cxx: UConnector.i $(BUILD_SRCDIR)/UConnector.h
	$(SWIG) -python -py3 -c++ -outcurrentdir -I$(BUILD_SRCDIR) UConnector.i

_UModbus3_la_SOURCES 	= UModbus.cc UModbus_wrap.cxx
_UModbus3_la_CXXFLAGS	= -I$(BUILD_SRCDIR) $(UNISET_CFLAGS) $(UNISET_EXT_CFLAGS) $(POCO_CFLAGS) $(PYTHON_CFLAGS)
_UModbus3_la_LDFLAGS 	= -module -avoid-version
_UModbus3_la_LIBADD 	= $(UNISET_LIBS) $(UNISET_EXT_LIBS) $(POCO_LIBS) $(PYTHON_LIBS)

UModbus_wrap.cxx: UInterface.i $(BUILD_SRCDIR)/UModbus.h
	$(SWIG) -python -py3 -c++ -outcurrentdir -I$(BUILD_SRCDIR) UModbus.i

_UExceptions3_la_SOURCES 	= UExceptions_wrap.cxx
_UExceptions3_la_CXXFLAGS	= -I$(BUILD_SRCDIR) $(UNISET_CFLAGS) $(UNISET_EXT_CFLAGS) $(PYTHON_CFLAGS)
_UExceptions3_la_LDFLAGS 	= -module -avoid-version
_UExceptions3_la_LIBADD 	= $(UNISET_LIBS) $(UNISET_EXT_LIBS) $(PYTHON_LIBS)

UExceptions_wrap.cxx: UExceptions.i $(BUILD_SRCDIR)/UExceptions.h
	$(SWIG) -python -py3 -c++ -outcurrentdir -I$(BUILD_SRCDIR) UExceptions.i

# Специальное правило для удаления сгенерированного кода
# для дальнейшей перегенерации
clean-gen:
	rm -rf *.py* *_wrap.cxx

$(BUILD_SRC):
	ln -sf $(BUILD_SRCDIR)/$@

all-local: $(BUILD_SRC)


include $(top_builddir)/include.mk

endif
