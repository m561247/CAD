NAME: musa 
DESCRIPTION: muli-level logic simulator for X10
TYPE: T
PACKAGES: oh oct list st utility port errtrap xg
INSTALL: ${CAD}/lib/musa
MISC: README USER_GATES tst leds

EXTRAFLAGS: 

SOURCES: alloc.c device.c errors.c gn.c hash.c info.c inter.c iroutines.c latch.c led.c logic.c musa.c memory.c merge.c my.c newlatch.c panel.c plot.c prop.c ram.c readoct.c readsim.c state.c tables.c trans.c vdd.c version.c yylex.c 7seg.c
HEADERS: musa.h x10-x11.h
OBJECTS: alloc.o device.o errors.o gn.o hash.o info.o inter.o iroutines.o latch.o led.o logic.o musa.o memory.o merge.o my.o newlatch.o panel.o plot.o prop.o ram.o readoct.o readsim.o state.o tables.o trans.o vdd.o version.o yylex.o 7seg.o
SYSLIBS: -lXMenu -lX -ltermcap -lm

VERBATIMSTART
test::
	@(cd tst; ${CADBIN}/${TARGET} -i segtest.musa segtest:bdnet)

${CAD}/lib/musa:leds
	rm -rf $@
	-mkdir $@
	${CP} -r $?/* $@

VERBATIMEND

TESTNAME: $(NAME)-test
TESTSOURCES: 
TESTOBJECTS: 
