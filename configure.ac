#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(ftrace, 0.93, BUG-REPORT-ADDRESS)
AC_CONFIG_SRCDIR([ftrace-test.c])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(dist-bzip2)

if test "x$prefix" != xNONE; then
  AC_DEFINE_UNQUOTED(FTRACE_LIB, "${prefix}/lib/ftrace", [libdir])
else
  AC_DEFINE_UNQUOTED(FTRACE_LIB, "${ac_default_prefix}/lib/ftrace", [libdir])
fi

CFLAGS=
# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

AC_CHECK_PROG(GLIB_CONFIG, glib-config, glib-config)
if test "x$GLIB_CONFIG" = x; then
    AC_MSG_ERROR([Please install libglib1.2 development package])
fi
CFLAGS="$CFLAGS `$GLIB_CONFIG --cflags`"
CPPFLAGS="$CFLAGS `$GLIB_CONFIG --cflags`"
LDFLAGS="$LDFLAGS `$GLIB_CONFIG --libs`"

# Checks for libraries.
AC_CHECK_LIB([glib], [g_malloc])
AC_CHECK_LIB([elf], [elf_version])
AC_CHECK_LIB([dl], [dlsym])
AC_CHECK_LIB([pthread], [pthread_self])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h sys/time.h syslog.h unistd.h])
AC_CHECK_HEADERS(glib.h,, AC_MSG_ERROR([Please install libglib development package]))
AC_CHECK_HEADERS(libelf.h,, AC_MSG_ERROR([Please install libelf development package]))
AC_CHECK_HEADERS([libdwarf/libdwarf.h libdwarf/dwarf.h],, AC_MSG_ERROR([Please install libdwarf development package]))

for f in /usr/local/lib/libdwarf.a /usr/lib/libdwarf.a; do
    DWARF_LIBS=$f;
    if test -f $DWARF_LIBS; then
        break;
    fi
done
AC_CHECK_FILE($DWARF_LIBS,, AC_MSG_ERROR([Please install libdwarf first.]))
AC_SUBST(DWARF_LIBS)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday localtime_r setenv])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
