## This must come first when building an application that uses Intel
## SGX enclaves. It can build against the Intel SGX SDK or Microsoft's
## OpenEnclave

include $(top_srcdir)/build-aux/sgx_tlib.am

libdir = $(prefix)/lib64
includedir = $(prefix)/include/escheme

include_HEADERS = e-scheme.h

lib_LIBRARIES = libsgx_tescheme.a
libsgx_tescheme_a_CFLAGS = -fno-builtin-memset -fPIC
libsgx_tescheme_a_SOURCES = e-scheme.c

CLEANFILES = $(lib_LIBRARIES)
