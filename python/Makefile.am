
# Build the swig text C++ code
# 
# Makefile vars set by the SWIG autoconf macros:
# SWIG: The swig compiler; set if one is found
# SWIG_LIB: Set to the swig library path
# AX_SWIG_PYTHON_CPPFLAGS: obvious
# AX_SWIG_PYTHON_OPT: SWIG options to generate code for Python
#
# jhrg 2/24/16

AUTOMAKE_OPTIONS = foreign check-news subdir-objects

AM_CPPFLAGS = -I. -I$(top_srcdir)/GNU $(XML2_CFLAGS) $(AX_SWIG_PYTHON_CPPFLAGS)
AM_CXXFLAGS = 

EXTRA_DIST = libdap.i

BUILT_SOURCES = libdap_wrap.cpp libdap.py
# _Shape.so in this dir is a symbolic link to the file in .libs
CLEANFILES = $(BUILT_SOURCES) *.pyc _libdap.so
# Might make CLEANFILE --> MAINTAINERCLEANFILES

pkgpython_PYTHON = libdap.py
pkgpyexec_LTLIBRARIES = _libdap.la

_libdap_la_SOURCES = libdap_wrap.cpp
_libdap_la_CPPFLAGS = $(AM_CPPFLAGS)
_libdap_la_LDFLAGS = -avoid-version -module 
# -export-symbols-regex initlibdap
_libdap_la_LIBADD = $(top_builddir)/libdap.la

all-local:
	if test ! -L _libdap.so; then $(LN_S) .libs/_libdap.so; fi

%_wrap.cpp: %.i
	$(SWIG) $(AX_SWIG_PYTHON_OPT) $(AM_CPPFLAGS) -o $@ $< 
