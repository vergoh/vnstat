#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_INIT([vnstat], [1.5_beta])
AM_INIT_AUTOMAKE([foreign subdir-objects std-options -Wall -Werror])
AC_CONFIG_SRCDIR([src/vnstat.c])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P

# Checks for libraries.
AC_CHECK_LIB([m], [pow])
AC_CHECK_LIB([gd], [gdImageLine], [IMAGELIBS=-lgd])
AC_SUBST([IMAGELIBS])
AM_CONDITIONAL([HAVE_LIBGD], [test "$IMAGELIBS" = "-lgd"])
PKG_CHECK_MODULES([CHECK], [check >= 0.9.6], [], [AC_MSG_NOTICE([testcases can not be execute without check installed])])
AM_CONDITIONAL([HAVE_CHECK], [test "$CHECK_LIBS"])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h locale.h stdint.h stdlib.h string.h sys/file.h sys/mount.h sys/param.h sys/socket.h sys/statvfs.h sys/time.h syslog.h unistd.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_INT32_T
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2 localtime_r memset mkdir pow rmdir setlocale strcasecmp strchr strdup strerror strncasecmp strstr strtol strtoul strtoull tzset])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
