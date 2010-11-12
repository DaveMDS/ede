AC_INIT(ede, 0.5, dave@gurumeditation.it)
AC_PREREQ([2.60])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_GNU_SOURCE
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_ISC_POSIX

AM_INIT_AUTOMAKE(1.6 dist-bzip2)
AM_CONFIG_HEADER(config.h)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])
define([AC_LIBTOOL_LANG_GCJ_CONFIG], [:])
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])
AC_PROG_LIBTOOL

VMAJ=`echo $PACKAGE_VERSION | awk -F. '{printf("%s", $1);}'`
VMIN=`echo $PACKAGE_VERSION | awk -F. '{printf("%s", $2);}'`
VMIC=`echo $PACKAGE_VERSION | awk -F. '{printf("%s", $3);}'`
SNAP=`echo $PACKAGE_VERSION | awk -F. '{printf("%s", $4);}'`
version_info=`expr $VMAJ + $VMIN`":$VMIC:$VMIN"
AC_SUBST(VMAJ)
AC_SUBST(version_info)

AC_C_BIGENDIAN
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_C_CONST
AC_FUNC_ALLOCA
AC_C___ATTRIBUTE__

ALL_LINGUAS=`cat po/LINGUAS | grep -v '^[ ]*#'`
AC_SUBST(ALL_LINGUAS)

AM_GNU_GETTEXT_VERSION([0.12.1])
AM_GNU_GETTEXT([external])

##################
# Mandatory Libs #
##################
PKG_CHECK_MODULES( [EDE],
   [eina >= 1.0]
   [evas >= 1.0]
   [ecore >= 1.0]
   [ecore-evas >= 1.0]
   [ecore-file >= 1.0]
   [edje >= 1.0]
)


##################
# Optional Libs  #
##################
#AC_E_CHECK_PKG([LIBXML2], [libxml-2.0])
#AM_CONDITIONAL(BUILD_LIBXML2, test BUILD_LIBXML2)


##################
# Edje cc        #
##################
AC_ARG_WITH([edje-cc],
   [AC_HELP_STRING([--with-edje-cc=PATH],
                   [specify a specific path to edje_cc])],
   [edje_cc=$withval;
    AC_MSG_NOTICE([edje_cc explicitly set to $edje_cc])
   ],[edje_cc=$(pkg-config --variable=prefix edje)/bin/edje_cc])
AC_SUBST(edje_cc)



##################
# Automake       #
##################
AC_OUTPUT([
Makefile
src/Makefile
src/bin/Makefile
data/Makefile
data/desktop/Makefile
data/themes/Makefile
data/themes/default/Makefile
data/levels/Makefile
data/towers/Makefile
po/Makefile.in
])


##################
# Final Report   #
##################
cat << EOF

$PACKAGE $VERSION configured with:

Flags:
   CFLAGS.....(C): $CFLAGS
   CXXFLAGS.(C++): $CXXFLAGS
   CPPFLAGS.(CPP): $CPPFLAGS
   LDFLAGS...(LD): $LDFLAGS

Installation Path:.......: $prefix
   Binaries..............: $prefix/bin
   Themes................: $prefix/share/ede/themes

Now type 'make' ('gmake' on some systems) to compile emc, if it
builds successfully then you can 'make install', acquiring required
permissions with 'su' or 'sudo'.

EOF

