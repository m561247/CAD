#
#  Upper level Makefile: 
# 	a TOOL-LIST file must be present in the current directory.
# 
#  Oct Tools Distribution 5.0
# 
#  Copyright (c) 1988, 1989, 1990, 1991 
#	Regents of the University of California.
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
# To build the system from scratch (using this Makefile), do the following:
#
#	make clean
#	make strip-depend
#	make header
#	make install
#------------------------------------------------------------------------------

# for HPUX
SHELL	= /bin/sh
MAKE = /bin/make

LINTCREATEFLAG = -C
LINTEXTRAS = 
OPTFLAG =
DBGFLAG =
CP = cp
AR = ar
RANLIB = ranlib
CPLUSPLUS = g++


# for the sequent
P = 

CAD	= ${OCTTOOLS}
UTILS	= ${CAD}/utils
XLIBLOC = -L/projects/X11/X11/mips/lib
XINCLOC = -I/projects/X11/X11/common/include

MAKEORDER= ${OCTTOOLS}/bin/make-order

MAKEVARS =	\
		"CAD=${CAD}" \
		"OCTTOOLS=${OCTTOOLS}" \
		"CC=${CC}" \
		"CP=${CP}" \
		"OPTFLAG=${OPTFLAG}" \
		"DBGFLAG=${DBGFLAG}" \
		"DISTHOST=${DISTHOST}" \
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
		"UTILS=${UTILS}" \
		"AR=${AR}" \
		"RANLIB=${RANLIB}" \
		"CPLUSPLUS=${CPLUSPLUS}" \
		"XINCLOC=${XINCLOC}" \
		"XLIBLOC=${XLIBLOC}" \
		"XAPPLOC=${XAPPLOC}" \
		"VPATH=${VPATH}"


# -------------------------------------------------------
install:
	-echo "# $(MAKE) $@" ;\
	 for x in `TOOL-LIST`; do \
	  echo "cd $$x ; $(MAKE) install" ; cd $$x ; \
	  $(MAKE) $(MFLAGS) $(MAKEVARS) install clean ;\
	  echo "cd .. # done in $$x ($@)" ; cd .. ; \
	 done

debug debug-g debug-pg header install.lint:
	-@echo "# $(MAKE) $@" ;\
	 for x in `TOOL-LIST`; do \
	  echo "cd $$x ; $(MAKE) $@" ; cd $$x ; \
	  $(MAKE) $(MFLAGS) $(MAKEVARS) $@ ;\
	  echo "cd .. # done in $$x ($@)" ; cd .. ; \
	 done

lint test:
	-@echo "# $(MAKE) $@" ;\
	 for x in `TOOL-LIST`; do \
	  echo "cd $$x ; $(MAKE) $@" ; cd $$x ; \
	  $(MAKE) $(MFLAGS) $(MAKEVARS) $@ clean ;\
	  echo "cd .. # done in $$x ($@)" ; cd .. ; \
	 done

build clean depend dist info print strip-depend tags uninstall TAGS::
	-@echo "# $(MAKE) $@" ;\
	 for x in `TOOL-LIST`; do \
	  echo "cd $$x ; $(MAKE) $@" ; cd $$x ; \
	  $(MAKE) $(MFLAGS) $(MAKEVARS) $@ ;\
	  echo "cd .. # done in $$x ($@)" ; cd .. ; \
	 done

dist::

check:
	-@${MAKEORDER} ${TOOLS}

tool:
	-@echo "# $(MAKE) $@" ;\
	 for x in `${MAKEORDER} ${TOOL}`; do \
	  echo "cd $$x ; $(MAKE) install" ; cd $$x ; \
	  $(MAKE) $(MFLAGS) $(MAKEVARS) install ;\
	  echo "cd .. # done in $$x (install)" ; cd .. ; \
	 done

installtool:
	-@echo "cd $(TOOL) ; $(MAKE) install clean" ; cd $(TOOL) ;\
	 $(MAKE) $(MFLAGS) $(MAKEVARS) install clean ;\
	 echo "cd .. # done in $(TOOL) (install clean)" ; cd ..

cleantool:
	-@echo "cd $(TOOL) ; $(MAKE) clean" ; cd $(TOOL) ;\
	 $(MAKE) $(MFLAGS) $(MAKEVARS) clean ;\
	 echo "cd .. # done in $(TOOL) (clean)" ; cd ..


help: 
	@more ${OCTTOOLS}/lib/utils/Makefile.help
