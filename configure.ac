#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_INIT([vnstat], [2.6])
AM_INIT_AUTOMAKE([foreign subdir-objects std-options -Wall -Werror])
AM_MAINTAINER_MODE

AC_CONFIG_SRCDIR([src/vnstatd.c])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P

# Extra options
AC_ARG_ENABLE([image-output], [
AS_HELP_STRING([--disable-image-output], [disable PNG image output])])

CFLAG_EXTRAS=""
AC_ARG_ENABLE([werror], [
AS_HELP_STRING([--enable-werror], [add -Werror to CFLAGS])], [CFLAG_EXTRAS="-Werror"])
AC_SUBST([CFLAG_EXTRAS])

AC_ARG_ENABLE([extra-paths], [
AS_HELP_STRING([--disable-extra-paths], [do not add extra paths to CFLAGS and LDFLAGS])], [], [
CFLAGS="$CFLAGS -I/usr/local/include" LDFLAGS="$LDFLAGS -L/usr/local/lib"])

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

# Check if being cross-compiled
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AM_CONDITIONAL([IS_CROSSCOMPILED], [test "$host" != "$build"])

# Check if 64-bit interface counters are used in Linux
AC_CHECK_DECLS([IFLA_STATS64], [], [], [[#include <linux/rtnetlink.h>]])

# Check if O_CLOEXEC can be used
AC_CHECK_DECLS([O_CLOEXEC], [], [], [[#include <fcntl.h>]])

# Check availability and level of SQLite WAL mode support
AC_CHECK_DECLS([SQLITE_CHECKPOINT_RESTART, SQLITE_CHECKPOINT_TRUNCATE], [], [], [[#include <sqlite3.h>]])

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
AC_FUNC_MKTIME
AC_CHECK_FUNCS([dup2 localtime_r memset mkdir pow rmdir setlocale strcasecmp strchr strdup strerror strncasecmp strstr strtol strtoul strtoull tzset mbstowcs wcswidth])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

AC_MSG_NOTICE()
AC_MSG_NOTICE([ ============================================== ])
AC_MSG_NOTICE()
AC_MSG_NOTICE([         host: $host ])
AC_MSG_NOTICE([        build: $build ])
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
