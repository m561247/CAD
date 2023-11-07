#
#  Octtools Makefile
#
#  crystal - A CMOS switch level timing analyser
#
#  Makefile created on Wed Apr 24 17:29:31 PDT 1991 by octtools (using 'create-octtools-makefile')
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

# CAD	= /projects/octtools/octtools-5.0/vax
UTILS	= ${OCTTOOLS}/utils
XINCLOC = -I/usr/local/X11R4/include
XLIBLOC = -L/usr/local/X11R4/lib

LINTCREATEFLAG = -C
LINTEXTRAS =
LDEXTRAS =
DBGFLAG =
OPTFLAG =

# an alternative if you want links in the installation locations
# back to the source directories would be:
#   CP = sh -c 'ln -s `pwd`/$$0 $$1'
CP = cp

BUILD = ${OCTTOOLS}/bin/create-octtools-makefile

# cute hack for the sequent
#   setenv PARALLEL 8; make P="&" install
P = 

NAME	= crystal

# packages required for this package/tool
REQUIRE	= oh oct st tr list errtrap uprintf utility mm port vov

# tools required for this tool
TOOLREQUIRE = 

SRC	= build.c check.c clear.c commands.c debug.c delay.c dprint.c expand.c flow.c graphout.c hash.c lookup.c main.c mark.c misc.c model.c nextfield.c octBuild.c octCap.c octNames.c octProps.c octStHelper.c octStore.c print.c prsmodel.c quad.c rcmodel.c rdspice.c runstats.c slopemodel.c spice.c totalcap.c version.c
LSRC	= build.c check.c clear.c commands.c debug.c delay.c dprint.c expand.c flow.c graphout.c hash.c lookup.c main.c mark.c misc.c model.c nextfield.c octBuild.c octCap.c octNames.c octProps.c octStHelper.c octStore.c print.c prsmodel.c quad.c rcmodel.c rdspice.c runstats.c slopemodel.c spice.c totalcap.c version.c
OBJ	= build.o check.o clear.o commands.o debug.o delay.o dprint.o expand.o flow.o graphout.o hash.o lookup.o main.o mark.o misc.o model.o nextfield.o octBuild.o octCap.o octNames.o octProps.o octStHelper.o octStore.o print.o prsmodel.o quad.o rcmodel.o runstats.o slopemodel.o spice.o totalcap.o version.o
HDR	= cOct.h crystal.h hash.h

CADBIN	= ${OCTTOOLS}/bin

TOOLINSTALL= ${CADBIN}/${TARGET} ${OCTTOOLS}/man/man1/${MAN}
INSTALL	= ${PKGINSTALL} ${TOOLINSTALL} ${OCTTOOLS}/bin/mkcp ${OCTTOOLS}/man/man1/mkcp.1 rdspice

DISTDEST= ${OCTTOOLS}/src/${NAME}
DISTHOST= BOGUS-HOST
MISC	= Makefile Makefile.template PROBLEMS README ReadMe changelog att.c kenner1 kenner2 mkcp.1 mkcp.c rdspice.c spice tests unitmodel.c

LIBS	= $(OCTTOOLS)/lib/liblist$(DBGFLAG).a $(OCTTOOLS)/lib/liboh$(DBGFLAG).a $(OCTTOOLS)/lib/liboct$(DBGFLAG).a $(OCTTOOLS)/lib/libtr$(DBGFLAG).a $(OCTTOOLS)/lib/libvov$(DBGFLAG).a $(OCTTOOLS)/lib/libst$(DBGFLAG).a $(OCTTOOLS)/lib/libutility$(DBGFLAG).a $(OCTTOOLS)/lib/libmm$(DBGFLAG).a $(OCTTOOLS)/lib/liberrtrap$(DBGFLAG).a $(OCTTOOLS)/lib/libuprintf$(DBGFLAG).a $(OCTTOOLS)/lib/libport$(DBGFLAG).a
LINTLIBS= $(OCTTOOLS)/lib/llib-loh.ln $(OCTTOOLS)/lib/llib-loct.ln $(OCTTOOLS)/lib/llib-lst.ln $(OCTTOOLS)/lib/llib-ltr.ln $(OCTTOOLS)/lib/llib-llist.ln $(OCTTOOLS)/lib/llib-lerrtrap.ln $(OCTTOOLS)/lib/llib-luprintf.ln $(OCTTOOLS)/lib/llib-lutility.ln $(OCTTOOLS)/lib/llib-lmm.ln $(OCTTOOLS)/lib/llib-lport.ln $(OCTTOOLS)/lib/llib-lvov.ln

TARGET	= ${NAME}
TARGETG	= ${NAME}-g
TARGETPG	= ${NAME}-pg
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

INCLUDE	= ${XINCLOC}  -I${OCTTOOLS}/include
CFLAGS	= ${OPTFLAG} ${DBGFLAG} ${INCLUDE} 
VERSION	= "-DCUR_DATE=\"`date | awk '{print $$2, $$3, $$6}'`\"" \
	  "-DCUR_TIME=\"`date | awk '{print $$4}'`\""
LINTFLAGS= ${INCLUDE} ${LINTEXTRAS}
LDFLAGS	= ${XLIBLOC} -lc -lm ${LDEXTRAS}
RDFLAGS    = -Rich
#-----------------------------------------------------------------------

prog: ${TARGET}

all: tags lint ${TARGET}

build: Makefile.template
	${BUILD} Makefile.template

install: ${INSTALL}
install.lint: ${LINTINSTALL}

uninstall:
	rm -rf ${INSTALL}

version.o: version.c
	${CC} ${CFLAGS} ${VERSION} -c version.c
	-touch -f version.c

debug: debug-g debug-pg


debug-g:
	rm -f ${OBJ}
	$(MAKE) $(MFLAGS) DBGFLAG=-g $(MAKEVARS) ${TARGETG}

debug-pg:
	rm -f ${OBJ}
	$(MAKE) $(MFLAGS) DBGFLAG=-pg $(MAKEVARS) ${TARGETPG}



${TARGET} ${TARGETG} ${TARGETPG}:${P} ${OBJ} ${LIBS}
	${CC} ${OPTFLAG} ${DBGFLAG} -o $@ ${OBJ} ${LIBS} ${LDFLAGS}


header::

${CADBIN}/${TARGET}: ${TARGET}
	rm -f $@
	${CP} $? $@
	strip $@

${OCTTOOLS}/man/man1/${MAN}: ${MAN}
	rm -f $@
	${TBL} < $? > $@

lint: ${LSRC} ${DRVRSRC} ${HDR} ${LINTLIBS}
	lint ${LINTFLAGS} ${LSRC} ${DRVRSRC} ${LINTLIBS} ${LDFLAGS} | tee lint

clean::
	@rm -f ${OBJ} ${DRVROBJ} ${TARGET} ${TARGETG} ${TARGETPG} ${DRIVER} 
	@rm -f tags TAGS ${LIB} ${LIBG} ${LIBPG} ${LINTLIB} lint 
	@rm -f make.out mktemp core .p .pg *~ __________ELEL_

require:
	@echo ${REQUIRE}

help:
	@more ${OCTTOOLS}/lib/utils/Makefile.help

toolrequire:
	@echo ${TOOLREQUIRE}

test::
	@test -f ${CADBIN}/${TARGET}

tags: ${LSRC} ${DRVRSRC} ${HDR}
	ctags ${LSRC} ${DRVRSRC} ${HDR}

TAGS: ${LSRC} ${DRVRSRC} ${HDR}
	etags ${LSRC} ${DRVRSRC} ${HDR}

info:
	@echo '${NAME}:  A CMOS switch level timing analyser'

print:: ${DOC} ${MAN}
	${TBL} < ${MAN} | ${TROFF} -P${PRINTER} ${MACROS}

dist: ${SRC} ${DRVRSRC} ${HDR} ${DOC} ${MAN}
	rdist ${RDFLAGS} ${SRC} ${DRVRSRC} ${HDR} ${DOC} ${MAN} ${MISC} ${DISTHOST}:${DISTDEST}

test-dist: ${SRC} ${DRVRSRC} ${HDR} ${DOC} ${MAN}
	rdist ${RDFLAGS} -hv ${SRC} ${DRVRSRC} ${HDR} ${DOC} ${MAN} ${MISC} ${DISTHOST}:${DISTDEST}


depend: ${SRC} ${DRVRSRC} ${HDR}
	@rm -f mktemp
	@sed '/^#--DO NOT CHANGE ANYTHING AFTER THIS LINE/,$$d' Makefile > mktemp
	@echo '#--DO NOT CHANGE ANYTHING AFTER THIS LINE' >> mktemp
	@${OCTTOOLS}/bin/cc-M ${INCLUDE} ${SRC} ${DRVRSRC} | sed 's|${OCTTOOLS}|$${OCTTOOLS}|g' >>mktemp
	@mv mktemp Makefile

strip-depend:
	@rm -f mktemp
	@sed '/^#--DO NOT CHANGE ANYTHING AFTER THIS LINE/,$$d' Makefile > mktemp
	@mv mktemp Makefile

# does not work with 'pat2tap' creation of devnames.h
#${SRC} ${DRVRSRC} ${HDR}:
#	co $@

#--EXTRA TARGETS

${OCTTOOLS}/bin/mkcp: mkcp
	${CP} $? $@

mkcp: mkcp.o nextfield.o
	${CC} ${COMPFLAGS} ${INCLUDE} -o $@ mkcp.o nextfield.o

${OCTTOOLS}/man/man1/mkcp.1: mkcp.1
	rm -f $@
	${TBL} < $? > $@

rdspice: rdspice.c
	${CC} ${COMPFLAGS} ${INCLUDE} $? -o $@

clean::
	rm -rf mkcp mkcp.o rdspice

mkcp.o: ${OCTTOOLS}/include/ansi.h
rdspice.o: ${OCTTOOLS}/include/ansi.h


