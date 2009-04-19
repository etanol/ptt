#
# Build rules for the tracing library
#
all: ptt.do pcc

# Automatically generated sources
reals.h: pthread.api mkreals.awk
	awk -f mkreals.awk pthread.api >$@

function_event.pcf: pthread.api mkpcf.awk
	awk -f mkpcf.awk pthread.api >$@

wrappers.c: pthread.api mkwrappers.awk
	awk -f mkwrappers.awk pthread.api >$@


# Library
ptt.o: core.o wrappers.o
	ld -r -o $@ $^

ptt.do: core.do wrappers.do
	ld -r -o $@ $^

core.o core.do: core.c core.h timestamp.h reals.h

wrappers.o wrappers.do: wrappers.c core.h ptt.h reals.h

%.o: %.c
	gcc -Wall -pipe -O3 -fomit-frame-pointer -c -o $@ $<

%.do: %.c
	gcc -Wall -pipe -O0 -c -o $@ $<

# Compiler wrapper
pcc: pthread.api pcc.in mkpcc.awk
	if : ; then \
	    cat pthread.api && \
	    echo '::@@::@@::' &&  \
	    cat pcc.in ; \
	fi | awk -f mkpcc.awk >$@ && chmod u+x $@


# Cleaning
.PHONY: clean distclean

clean:
	-rm -f *.o *.do

distclean: clean
	-rm -f reals.h function_event.pcf wrappers.c pcc

