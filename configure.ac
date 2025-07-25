AC_INIT(CMM, 19.1.0)
AM_INIT_AUTOMAKE

AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_CONFIG_MACRO_DIRS([m4])

AC_ARG_WITH([cpal],
	    AS_HELP_STRING([--with-cpal=bpf], [Use libbpf backend for CPAL]),
	    [], [cpal=fci])
AM_CONDITIONAL([CPAL_BPF], [test $with_cpal = "bpf"])


package=cmm

LIBNFCONNTRACK_REQUIRED=0.0.16
PKG_CHECK_MODULES(LIBNFCONNTRACK, libnetfilter_conntrack >= $LIBNFCONNTRACK_REQUIRED,,
	AC_MSG_ERROR(Cannot find libnetfilter_conntrack >= $LIBNFNCONNTRACK_REQUIRED))

AC_MSG_CHECKING([for BASELIB environment variable])
if test -n "$BASELIB"; then
  # Use the value from the environment if it exists
  libdir_suffix="$BASELIB"
else
  # Default to 'lib' if the variable is not set
  libdir_suffix="lib"
fi
AC_MSG_RESULT([$libdir_suffix])

# Define a new directory variable for Automake
baselibdir="${libdir}"
if test "$libdir_suffix" != "lib"; then
  baselibdir='${exec_prefix}/'"$libdir_suffix"
fi
AC_SUBST(baselibdir)

CFLAGS="$CFLAGS $LIBNFCONNTRACK_CFLAGS"
CMM_LIBS="$LIBNFCONNTRACK_LIBS"
AC_SUBST(CMM_LIBS)

AC_CONFIG_FILES([
	Makefile
	src/Makefile
])
AC_OUTPUT
