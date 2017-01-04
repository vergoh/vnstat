#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_INIT([vnstat], [2.0_alpha])
AM_INIT_AUTOMAKE([foreign subdir-objects std-options -Wall -Werror])
AM_MAINTAINER_MODE

AC_CONFIG_SRCDIR([src/vnstatd.c])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P

AC_ARG_ENABLE([image-output], [
AS_HELP_STRING([--disable-image-output], [disable PNG image output])])

# Checks for libraries.
AC_CHECK_LIB([m], [pow], [], [AC_MSG_ERROR([could not find required math library])])
AC_CHECK_LIB([sqlite3], [sqlite3_prepare_v2], [], [AC_MSG_ERROR([could not find required sqlite3 library])])

AS_IF([test "x$enable_image_output" != "xno"], [
AC_CHECK_LIB([gd], [gdImagePng], [IMAGELIBS=-lgd])
AC_SUBST([IMAGELIBS])])

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
AC_CHECK_FUNCS([dup2 localtime_r memset mkdir pow rmdir setlocale strcasecmp strchr strdup strerror strncasecmp strstr strtol strtoul strtoull tzset mbstowcs wcswidth])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

AC_MSG_NOTICE()
AC_MSG_NOTICE([ ============================================== ])
AC_MSG_NOTICE()
AC_MSG_NOTICE([       prefix: $prefix     ])
AC_MSG_NOTICE([   sysconfdir: $sysconfdir ])
AC_MSG_NOTICE()
AC_MSG_NOTICE([   Install paths ])
AC_MSG_NOTICE()
AC_MSG_NOTICE([    vnstat: $prefix/bin/vnstat ])

AM_COND_IF([HAVE_LIBGD], [
AC_MSG_NOTICE([   vnstati: $prefix/bin/vnstati ])])

AC_MSG_NOTICE([   vnstatd: $prefix/sbin/vnstatd ])
AC_MSG_NOTICE([    config: $sysconfdir/vnstat.conf ])
AC_MSG_NOTICE()
AM_COND_IF([HAVE_CHECK], [
AC_MSG_NOTICE([          Tests: available ])], [
AC_MSG_NOTICE([          Tests: unavailable ])])
AM_COND_IF([HAVE_LIBGD], [
AC_MSG_NOTICE([   Image output: enabled ])], [
AC_MSG_NOTICE([   Image output: disabled ])])
AC_MSG_NOTICE()
AC_MSG_NOTICE([ ============================================== ])
AC_MSG_NOTICE()
