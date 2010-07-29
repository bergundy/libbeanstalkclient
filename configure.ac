#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([libbeanstalkclient], [0.2], [roey.berman@gmail.com])
AC_CONFIG_SRCDIR([src/beanstalkclient.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([color-tests foreign -Wall -Werror])
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

LT_INIT

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

AC_SUBST(LIBTOOL_DEPS)

# Checks for libraries.
AC_CHECK_LIB([beanstalkproto], [bsp_gen_put_hdr])
AC_CHECK_LIB([check], [tcase_create])
AC_CHECK_LIB([ioqueue], [ioq_new])
AC_CHECK_LIB([sockutils], [tcp_client])

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memchr strdup])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tests/Makefile])

AC_CONFIG_MACRO_DIR([m4])
AC_OUTPUT
