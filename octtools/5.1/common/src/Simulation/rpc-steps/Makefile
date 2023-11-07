#
#  Octtools Makefile
#
#  rpc-steps - Spice Text Editing Package utilizing Schematics
#
#  Makefile created on Fri Sep 20 14:48:59 PDT 1991 by octtools (using 'create-octtools-makefile')
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

# CAD	= /vol/canova/canova2/octtools/octtools/mips
UTILS	= ${OCTTOOLS}/utils
# This works OK at UC Berkeley.
XINCLOC = -I/projects/X11/X11/common/include
XLIBLOC = -L/projects/X11/X11/mips/lib

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

NAME	= rpc-steps

# packages required for this package/tool
REQUIRE	= mm port rpc list st tr utility errtrap uprintf oh pep da th

# tools required for this tool
TOOLREQUIRE = 

SRC	= node.c oct2spicelib.c options.c rpc-spice.c dialogs.c table.c sources.c edit.c terminals.c changes.c
LSRC	= node.c oct2spicelib.c options.c rpc-spice.c dialogs.c table.c sources.c edit.c terminals.c changes.c
OBJ	= node.o oct2spicelib.o options.o rpc-spice.o dialogs.o table.o sources.o edit.o terminals.o changes.o
HDR	= defs.h table.h

CADBIN	= ${OCTTOOLS}/bin/rpc

TOOLINSTALL= ${CADBIN}/${TARGET} ${OCTTOOLS}/man/man1/${MAN}
INSTALL	= ${PKGINSTALL} ${TOOLINSTALL} 

DISTDEST= ${OCTTOOLS}/src/${NAME}
DISTHOST= BOGUS-HOST
MISC	= Makefile Makefile.template steps_setup

LIBS	= $(OCTTOOLS)/lib/libpep$(DBGFLAG).a $(OCTTOOLS)/lib/libregion$(DBGFLAG).a $(OCTTOOLS)/lib/libgu$(DBGFLAG).a $(OCTTOOLS)/lib/libkd$(DBGFLAG).a $(OCTTOOLS)/lib/libth$(DBGFLAG).a $(OCTTOOLS)/lib/libtap$(DBGFLAG).a $(OCTTOOLS)/lib/liboh$(DBGFLAG).a $(OCTTOOLS)/lib/libvov$(DBGFLAG).a $(OCTTOOLS)/lib/liboptions$(DBGFLAG).a $(OCTTOOLS)/lib/libda$(DBGFLAG).a $(OCTTOOLS)/lib/librpc$(DBGFLAG).a $(OCTTOOLS)/lib/liboct$(DBGFLAG).a $(OCTTOOLS)/lib/libtr$(DBGFLAG).a $(OCTTOOLS)/lib/libst$(DBGFLAG).a $(OCTTOOLS)/lib/libutility$(DBGFLAG).a $(OCTTOOLS)/lib/liblist$(DBGFLAG).a $(OCTTOOLS)/lib/libmm$(DBGFLAG).a $(OCTTOOLS)/lib/liberrtrap$(DBGFLAG).a $(OCTTOOLS)/lib/libuprintf$(DBGFLAG).a $(OCTTOOLS)/lib/libport$(DBGFLAG).a
LINTLIBS= $(OCTTOOLS)/lib/llib-lmm.ln $(OCTTOOLS)/lib/llib-lport.ln $(OCTTOOLS)/lib/llib-lrpc.ln $(OCTTOOLS)/lib/llib-llist.ln $(OCTTOOLS)/lib/llib-lst.ln $(OCTTOOLS)/lib/llib-ltr.ln $(OCTTOOLS)/lib/llib-lutility.ln $(OCTTOOLS)/lib/llib-lerrtrap.ln $(OCTTOOLS)/lib/llib-luprintf.ln $(OCTTOOLS)/lib/llib-loh.ln $(OCTTOOLS)/lib/llib-lpep.ln $(OCTTOOLS)/lib/llib-lda.ln $(OCTTOOLS)/lib/llib-lth.ln

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

INCLUDE	=  -I${OCTTOOLS}/include ${XINCLOC} 
CFLAGS	= ${OPTFLAG} ${DBGFLAG} ${INCLUDE} 
VERSION	= "-DCUR_DATE=\"`date | awk '{print $$2, $$3, $$6}'`\"" \
	  "-DCUR_TIME=\"`date | awk '{print $$4}'`\""
LINTFLAGS= ${INCLUDE} ${LINTEXTRAS}
LDFLAGS	= ${XLIBLOC} -lX11 -lm  ${LDEXTRAS}
RDFLAGS    = -Rich
#-----------------------------------------------------------------------

prog: ${TARGET}

all: tags lint ${TARGET}

build: Makefile.template
	@${BUILD} Makefile.template

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

${OCTTOOLS}/man/man1/${MAN}: ${MAN}
	rm -f $@
	${TBL} < $? > $@

lint: ${LSRC} ${DRVRSRC} ${HDR} ${LINTLIBS}
	lint ${LINTFLAGS} ${LSRC} ${DRVRSRC} ${LINTLIBS} ${LDFLAGS} | tee lint

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

tags: ${LSRC} ${DRVRSRC} ${HDR}
	ctags ${LSRC} ${DRVRSRC} ${HDR}

TAGS: ${LSRC} ${DRVRSRC} ${HDR}
	etags ${LSRC} ${DRVRSRC} ${HDR}

info:
	@echo '${NAME}:  Spice Text Editing Package utilizing Schematics'

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

