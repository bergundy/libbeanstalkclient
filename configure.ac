AC_PREREQ([2.59])
AC_INIT([libbeanstalkclient], [1.0], [roey.berman@gmail.com])
AC_CONFIG_SRCDIR([src/beanstalkclient.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_ARG_ENABLE(tests,[  --enable-tests          build tests],[ 
	PKG_CHECK_MODULES([CHECK], [check >= 0.9.4]) 
	build_tests=true
]
)

AM_CONDITIONAL([BUILD_TESTS], [test x$build_tests = xtrue ])

LT_INIT

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

AC_SUBST(LIBTOOL_DEPS)

# Checks for libraries.
AC_CHECK_LIB([ioqueue], [ioq_new])
AC_CHECK_LIB([sockutils], [tcp_client])

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdlib.h string.h sys/socket.h unistd.h arrayqueue.h])

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
AC_FUNC_STRTOD
AC_CHECK_FUNCS([strchr strdup strndup strtoul])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tests/Makefile])

AC_CONFIG_MACRO_DIR([m4])
AC_OUTPUT
