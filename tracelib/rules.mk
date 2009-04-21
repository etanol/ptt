#
# Build rules for the programs linking the tracing library
#

MAKEFLAGS += -r

# Library file listings
auto_sources  := reals.h wrappers.c event.pcf ldargs.mk
trace_objs    := core.o wrappers.o
trace_dbgs    := core.dbo wrappers.dbo
core          := core.o core.dbo
core_deps     := core.c core.h timestamp.h reals.h
wrappers      := wrappers.o wrappers.dbo
wrappers_deps := wrappers.c core.h ptt.h reals.h


all  : $(PROGRAMS)
debug: $(PROGRAMS:=.dbg)


# Build rule generator
define gen_build_rules
$(1)_OBJ := $(patsubst %.c,%.o,$(filter %.c,$($(1)_SOURCES)))
$(1)_DBG := $(patsubst %.c,%.dbo,$(filter %.c,$($(1)_SOURCES)))

objects += $$($(1)_OBJ) $$($(1)_DBG)

$(1): $$($(1)_OBJ) $(addprefix $(TRPATH)/,$(trace_objs))
	@echo 'gcc -pipe -Wl,-O1,-s -o $$@ $$^ -wrap_pthread $(addprefix -l,$($(1)_LIBS))' ; \
	gcc -pipe -Wl,-O1,-s $$(ld_wrap) -o $$@ $$^ -pthread $(addprefix -l,$($(1)_LIBS))

$(1).dbg: $$($(1)_DBG) $(addprefix $(TRPATH)/,$(trace_dbgs))
	@echo 'gcc -pipe -g -o $$@ $$^ -wrap_pthread $(addprefix -l,$($(1)_LIBS))' ; \
	gcc -pipe -g $$(ld_wrap) -o $$@ $$^ -pthread $(addprefix -l,$($(1)_LIBS))

$$($(1)_OBJ): $(filter %.h,$($(1)_SOURCES))
$$($(1)_DBG): $(filter %.h,$($(1)_SOURCES))
endef


# Source rule generator
define gen_source_rules
$(TRPATH)/$(1): $(TRPATH)/pthread.api $(TRPATH)/mk$(basename $(1)).awk
	awk -f $(TRPATH)/mk$(basename $(1)).awk $(TRPATH)/pthread.api >$$@
endef


# Perform rule generation
$(foreach p,$(PROGRAMS),$(eval $(call gen_build_rules,$(p))))
$(foreach s,$(auto_sources),$(eval $(call gen_source_rules,$(s))))


# Include generated Makefile or force its generation.
-include $(TRPATH)/ldargs.mk


# Pattern rules
.SUFFIXES: .c .o .dbo

%.o: %.c
	gcc -Wall -pipe -I$(TRPATH) -O3 -fomit-frame-pointer -c -o $@ $<

%.dbo: %.c
	gcc -Wall -pipe -I$(TRPATH) -DDEBUG -O0 -g -c -o $@ $<


# Tracing library specific dependencies
$(addprefix $(TRPATH)/,$(core)): $(addprefix $(TRPATH)/,$(core_deps))

$(addprefix $(TRPATH)/,$(wrappers)): $(addprefix $(TRPATH)/,$(wrappers_deps))


# Cleaning
.PHONY: clean distclean

clean:
	-rm -f $(objects) $(PROGRAMS) $(PROGRAMS:=.dbg)

distclean: clean
	-rm -f $(addprefix $(TRPATH)/, $(trace_objs) $(trace_dbgs) $(auto_sources))

