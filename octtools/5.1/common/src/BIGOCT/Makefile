### Big Octtools Library
# This offers an easier way to link a lot of octtools libraries.
#



TARGET = libBIGOCT.a

LIBS =  \
	${OCTTOOLS}/lib/libMaxport.a \
	${OCTTOOLS}/lib/libTable.a \
	${OCTTOOLS}/lib/libTgl.a \
	${OCTTOOLS}/lib/libdds.a \
	${OCTTOOLS}/lib/libsymlib.a \
	${OCTTOOLS}/lib/libregion.a \
	${OCTTOOLS}/lib/libgu.a \
	${OCTTOOLS}/lib/libtap.a \
	${OCTTOOLS}/lib/liboh.a \
	${OCTTOOLS}/lib/liboct.a \
	${OCTTOOLS}/lib/libtr.a \
	${OCTTOOLS}/lib/libkd.a \
	${OCTTOOLS}/lib/liboptions.a \
	${OCTTOOLS}/lib/libutility.a \
	${OCTTOOLS}/lib/liblist.a \
	${OCTTOOLS}/lib/libst.a \
	${OCTTOOLS}/lib/libmm.a \
	${OCTTOOLS}/lib/libport.a \
	${OCTTOOLS}/lib/liberrtrap.a \
	${OCTTOOLS}/lib/libuprintf.a \
	${OCTTOOLS}/lib/libvulcan.a  \
	${OCTTOOLS}/lib/libharpoon.a \
	${OCTTOOLS}/lib/libfang.a \
	${OCTTOOLS}/lib/libvov.a 

PKGINSTALL = ${OCTTOOLS}/lib/${TARGET}
MANINSTALL = ${OCTTOOLS}/man/man3/BIGOCT.3
# ------------------------------------------

${TARGET}:  ${LIBS}
	rm -f ${TARGET} 
	@mergelibs ${LIBS} 
	mv library ${TARGET}
	ranlib ${TARGET}

clean : 
	rm -f ${TARGET} *~


install : ${PKGINSTALL} ${MANINSTALL} 

${PKGINSTALL} : ${TARGET}
	rm -f ${PKGINSTALL}
	${CP} ${TARGET} ${PKGINSTALL}
	

${MANINSTALL} : BIGOCT.3
	${CP} BIGOCT.3 ${MANINSTALL}
	
depend strip-depend header:
	@echo "no rule"
