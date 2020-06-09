AC_INIT(libe-scheme, 0.1, cyx.xjtu@gmail.com)

AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC
AC_PROG_CPP
AC_PROG_RANLIB
AM_PROG_CC_C_O

CFLAGS = "-Wall -W -Wfloat-equal -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wredundant-decls \
-Wendif-labels -Wshadow -pipe -ffast-math -U__STRICT_ANSI__ -std=gnu99 ${ac_cv_sgx_tlib_cppflags} -nostdinc"

SGX_INIT()

SGX_TSTDC_CHECK_HEADERS([stdio.h])
SGX_TSTDC_CHECK_FUNCS([fprintf])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT()
