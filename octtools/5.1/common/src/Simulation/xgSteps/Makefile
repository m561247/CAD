#
#  Octtools Makefile
#
#  xgSteps - 
#
#  Makefile created on Fri Jul 26 15:03:17 PDT 1991 by octtools (using 'create-octtools-makefile')
#
#  Copyright (c) 1988, 1989, 1990, Regents of the University of California
#
#  For a description of the 'targets and variables': say "make help"
#------------------------------------------------------------------------------
.SUFFIXES: .c .y .l .cc .o

.cc.o:
	${CPLUSPLUS} ${CFLAGS} -c $*.cc

# for HPUX
SHELL	= /bin/sh

MAKE = /bin/make
AR = ar
RANLIB = ranlib
CPLUSPLUS = g++

# CAD	= /projects/octtools/octtools-5.1/mips
UTILS	= ${OCTTOOLS}/utils
XINCLOC = -I/usr/local/X11R4/include
XLIBLOC = -L/usr/local/X11R4/lib

LINTEXTRAS =
LDEXTRAS =

# an alternative if you want links in the installation locations
# back to the source directories would be:
#   CP = sh -c 'ln -s `pwd`/$$0 $$1'
CP = cp

BUILD = ${OCTTOOLS}/bin/create-octtools-makefile

# cute hack for the sequent
#   setenv PARALLEL 8; make P="&" install
P = 

NAME	= xgSteps

# packages required for this package/tool
REQUIRE	= mm port

# tools required for this tool
TOOLREQUIRE = 

SRC	= xgSteps.sh

CADBIN	= ${OCTTOOLS}/bin

TOOLINSTALL= ${CADBIN}/${TARGET} ${OCTTOOLS}/man/man1/${MAN}
INSTALL	= ${PKGINSTALL} ${TOOLINSTALL} 

DISTDEST= ${OCTTOOLS}/src/${NAME}
DISTHOST= BOGUS-HOST
MISC	= Makefile Makefile.template 


TARGET	= ${NAME}
MAN	= ${NAME}.1

PRINTER	= lps40
MACROS	= -man.4.3
PRINT	= lpr
TROFF	= ptroff
TBL	= tbl

MAKEVARS =	\
		"CAD=${CAD}" \
		"OCTTOOLS=${OCTTOOLS}" \
		"CC=${CC}" \
		"OPTFLAG=${OPTFLAG}" \
		"DBGFLAG=${DBGFLAG}" \
		"RDFLAGS=${RDFLAGS}" \
		"CP=${CP}" \
		"LINTCREATEFLAG=${LINTCREATEFLAG}" \
		"LDEXTRAS=${LDEXTRAS}" \
		"LINTEXTRAS=${LINTEXTRAS}" \
		"MAKE=$(MAKE)" \
		"MACROS=${MACROS}" \
		"P=${P}" \
		"PRINT=${PRINT}" \
		"PRINTER=${PRINTER}" \
		"TBL=${TBL}" \
		"TROFF=${TROFF}" \
		"AR=${AR}" \
		"RANLIB=${RANLIB}" \
		"CPLUSPLUS=${CPLUSPLUS}" \
		"XINCLOC=${XINCLOC}" \
		"XLIBLOC=${XLIBLOC}" \
		"VPATH=${VPATH}"

#-----------------------------------------------------------------------

prog: ${TARGET}

all: ${TARGET}

build: Makefile.template
	@${BUILD} Makefile.template

install: ${INSTALL}

install.lint: ${LINTINSTALL}

uninstall:
	rm -rf ${INSTALL}


debug: debug-g debug-pg



debug-g debug-pg:
	@echo no rule



${TARGET}: ${SRC}
	rm -f $@
	sed -e 's@_CADRUNDIR_@${CADROOT}@' \
	    -e 's@_CADDESTDIR_@${OCTTOOLS}@' < ${SRC} > $@
	chmod ugo+x $@

header::

${CADBIN}/${TARGET}: ${TARGET}
	rm -f $@
	${CP} $? $@

${OCTTOOLS}/man/man1/${MAN}: ${MAN}
	rm -f $@
	${TBL} < $? > $@

lint:
	@echo no rule

clean::
	@rm -f ${OBJ} ${DRVROBJ} ${TARGET} ${TARGETG} ${TARGETPG} ${DRIVER} 
	@rm -f tags TAGS ${LIB} ${LIBG} ${LIBPG} ${LINTLIB} lint 
	@rm -f make.out mktemp core .g .pg *~ __________ELEL_

require:
	@echo ${REQUIRE}

help:
	@more ${OCTTOOLS}/lib/utils/Makefile.help

toolrequire:
	@echo ${TOOLREQUIRE}

test::
	@test -f ${CADBIN}/${TARGET}



info:
	@echo '${NAME}:  '

print:: ${DOC} ${MAN}
	${TBL} < ${MAN} | ${TROFF} -P${PRINTER} ${MACROS}

dist: ${SRC} ${DRVRSRC} ${HDR} ${DOC} ${MAN}
	rdist ${RDFLAGS} ${SRC} ${DRVRSRC} ${HDR} ${DOC} ${MAN} ${MISC} ${DISTHOST}:${DISTDEST}

test-dist: ${SRC} ${DRVRSRC} ${HDR} ${DOC} ${MAN}
	rdist ${RDFLAGS} -hv ${SRC} ${DRVRSRC} ${HDR} ${DOC} ${MAN} ${MISC} ${DISTHOST}:${DISTDEST}

depend strip-depend:
	@echo no rule



# does not work with 'pat2tap' creation of devnames.h
#${SRC} ${DRVRSRC} ${HDR}:
#	co $@

#--EXTRA TARGETS

#--DO NOT CHANGE ANYTHING AFTER THIS LINE
