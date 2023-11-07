#
# 
#  Oct Tools Distribution
# 
#  Copyright (c) 1988, 1989, 1990, 1991
#	 Regents of the University of California.
#  All rights reserved.
# 
#  Use and copying of this software and preparation of derivative works
#  based upon this software are permitted.  However, any distribution of
#  this software or derivative works must include the above copyright
#  notice.
# 
#  This software is made available AS IS, and neither the Electronics
#  Research Laboratory or the University of California make any
#  warranty about the software, its performance or its conformity to
#  any specification.
# 
#  Suggestions, comments, or improvements are welcome and should be
#  addressed to:
# 
#    octtools@eros.berkeley.edu
#    ..!ucbvax!eros!octtools
# 
#
#  INSTALLATION INSTRUCTIONS
#

SHELL = /bin/sh
MAKE = /bin/make

LINTCREATEFLAG = -C
# -h for Ultrix/Mips, SYSV machines
LINTEXTRAS = 
LDEXTRAS =
OPTFLAG =
DBGFLAG =
CP = cp

# set to "&" for the sequent (speeds up the build)
P =

PRINTER = lps40
TROFF = ptroff
TBL = tbl
MACROS = -man.4.3
PRINT = lpr -p
AR = ar
RANLIB = ranlib
CPLUSPLUS = g++


UTILS = ${OCTTOOLS}/utils
INCLUDE = -I${OCTTOOLS}/include
MAKEFILE = Makefile

CREATEDIR = \
	${OCTTOOLS}/bin	\
	${OCTTOOLS}/bin/rpc	\
	${OCTTOOLS}/lib	

LINKDIR = \
	${OCTTOOLS}/include	\
	${OCTTOOLS}/lib/vem	\
	${OCTTOOLS}/lib/vov	\
	${OCTTOOLS}/doc	\
	${OCTTOOLS}/document	\
	${OCTTOOLS}/unsupported	\
	${OCTTOOLS}/man	

MAKEVARS =	\
		"OCTTOOLS=${OCTTOOLS}" \
		"INCLUDE=${INCLUDE}" \
		"CC=${CC}" \
		"CP=${CP}" \
		"DBGFLAG=${DBGFLAG}" \
		"OPTFLAG=${OPTFLAG}" \
	        "LDEXTRAS=${LDEXTRAS}" \
		"LINTEXTRAS=${LINTEXTRAS}" \
		"LINTCREATEFLAG=${LINTCREATEFLAG}" \
		"MAKE=$(MAKE)" \
		"MACROS=${MACROS}" \
		"P=${P}" \
		"PRINT=${PRINT}" \
		"PRINTER=${PRINTER}" \
		"TBL=${TBL}" \
		"TROFF=${TROFF}" \
		"UTILS=${UTILS}" \
		"AR=${AR}" \
		"RANLIB=${RANLIB}" \
		"CPLUSPLUS=${CPLUSPLUS}" \
		"XINCLOC=${XINCLOC}" \
		"XLIBLOC=${XLIBLOC}" \
		"XAPPLOC=${XAPPLOC}" \
		"VPATH=${VPATH}"


all:
	@echo "Please specify an explicit target"

nothing: ;

preinstall: ${DIRECTORIES} 
	@echo Started `date`; df .; 
	df .;\
	echo Ended `date`

build debug install:: preinstall header

dist::
	-rdist -c .cshrc .login INSTALL CHANGES COPYRIGHTS Makefile LETTERS ANNOUNCE README README-4.0 CLEAN-DIST blurb.ms ${DISTHOST}:${CAD}
	echo "cd document ; $(MAKE) $@" ; cd document ;\
	$(MAKE) $(MFLAGS) $(MAKEVARS) $@ ;\
	echo "cd .. # done in document ($@)" ; cd .. ;
	echo "cd unsupported ; $(MAKE) $@" ; cd unsupported ;\
	$(MAKE) $(MFLAGS) $(MAKEVARS) $@ ;\
	echo "cd .. # done in unsupported ($@)" ; cd .. ;
#	(cd tech; rdist -d CAD=${CAD} -d HOST=${DISTHOST} -f .distfile)
	-rdist -c tech ${DISTHOST}:${CAD}/tech
	-rdist -c demo ${DISTHOST}:${CAD}/demo

clean dist test uninstall build debug debug-g debug-pg install  install.lint header strip-depend::
	@echo Started `date`; df .; echo "cd src ; $(MAKE) $@" ; cd src ;\
	$(MAKE) $(MFLAGS) -f ${MAKEFILE} $(MAKEVARS) $@ ;\
	echo "cd .. # done in src ($@)" ; cd .. ;\
	df .;\
	echo Ended `date`

tool: preinstall header
	@echo Started `date`; df .; echo "cd src ; $(MAKE) TOOL=${TOOL} tool" ; cd src ;\
	$(MAKE) $(MFLAGS) -f ${MAKEFILE} $(MAKEVARS) TOOL=${TOOL} tool ;\
	echo "cd .. # done in src ($@)" ; cd .. ;\
	df .;\
	echo Ended `date`





