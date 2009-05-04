#
# Build rules for the programs linking the tracing library
#

MAKEFLAGS += -r

GCC           := gcc -pipe
CFLAGS        ?= -O3 -fomit-frame-pointer
CFLAGS_DBG    ?= -O0 -g
LINKFLAGS     ?= -Wl,-O1,-s
LINKFLAGS_DBG ?= -g

DEFS   := -D_REENTRANT -D_XOPEN_SOURCE=700
LDWRAP := -Wl,--wrap,pthread_create


# Default and shortcut rules
all     : traced
traced  : $(PROGRAMS)
untraced: $(PROGRAMS:=.untraced)
debug   : $(PROGRAMS:=.debug)


###########################  TRACING LIBRARY RULES  ###########################

# File listings
ptt_headers := ptt.h intestine.h timestamp.h
ptt_sources := core.c event.c wrappers.c postprocess.c
ptt_userapi := ptt.h
ptt_stub    := stub.h
ptt_object  := ptt.o
ptt_debug   := ptt.go
ptt_pcf     := basic.pcf

# Prepend proper path to all files
vars := headers sources userapi stub object debug pcf strizer
$(foreach v,$(vars),$(eval ptt_$(v) := $(addprefix $(PTT_PATH)/,$(ptt_$(v)))))

# Build rules
$(ptt_object): $(ptt_sources:.c=.o)
	ld -i -o $@ $^

$(ptt_debug): $(ptt_sources:.c=.go)
	ld -i -o $@ $^

$(ptt_sources:.c=.o): %.o: %.c $(ptt_headers)
	$(GCC) $(DEFS) $(CFLAGS) -c -o $@ $<

$(ptt_sources:.c=.go): %.go: %.c $(ptt_headers)
	$(GCC) -DDEBUG $(DEFS) $(CFLAGS_DBG) -c -o $@ $<

# AWK code to stringize the PCF
ptt_stringize := BEGIN { print "const char *PttPCF = " } \
                       { print "\"" $$$$0 "\\n\"" } \
                 END   { print ";" }


############################  USER PROGRAMS RULES  ############################

# Build rule generator
define gen_build_rules
$(1)_OBJ := $(patsubst %.c,%.o,$(filter %.c,$($(1)_SOURCES)) pcf_$(1).c)
$(1)_UNT := $(patsubst %.c,%.uo,$(filter %.c,$($(1)_SOURCES)))
$(1)_DBG := $(patsubst %.c,%.go,$(filter %.c,$($(1)_SOURCES)) pcf_$(1).c)

objects += $$($(1)_OBJ) $$($(1)_UNT) $$($(1)_DBG)
autopcf += pcf_$(1).c

$(1): $$($(1)_OBJ) $(ptt_object)
	$(GCC) $(LDWRAP) $(LINKFLAGS) -o $$@ $$^ -pthread $(addprefix -l,$($(1)_LIBS))

$(1).untraced: $$($(1)_UNT)
	$(GCC) $(LINKFLAGS) -o $$@ $$^ -pthread $(addprefix -l,$($(1)_LIBS))

$(1).debug: $$($(1)_DBG) $(ptt_debug)
	$(GCC) $(LDWRAP) $(LINKFLAGS_DBG) -o $$@ $$^ -pthread $(addprefix -l,$($(1)_LIBS))

$$($(1)_OBJ): %.o: %.c $(filter %.h,$($(1)_SOURCES))
	$(GCC) $(DEFS) -include $(ptt_userapi) $(CFLAGS) -c -o $$@ $$<

$$($(1)_UNT): %.uo: %.c $(filter %.h,$($(1)_SOURCES))
	$(GCC) $(DEFS) -include $(ptt_stub) $(CFLAGS) -c -o $$@ $$<

$$($(1)_DBG): %.go: %.c $(filter %.h,$($(1)_SOURCES))
	$(GCC) $(DEFS) -include $(ptt_userapi) $(CFLAGS_DBG) -c -o $$@ $$<

pcf_$(1).c: $(ptt_pcf) $(PCF_FILES) $$($(1)_PCF)
	awk '$(ptt_stringize)' $$^ >$$@
endef

# Perform rule generation
$(foreach p,$(PROGRAMS),$(eval $(call gen_build_rules,$(p))))


################################  COMMON RULES  ################################

# Cleaning
.PHONY: clean distclean

clean:
	-rm -f $(objects) $(PROGRAMS) $(PROGRAMS:=.untraced) $(PROGRAMS:=.debug)

distclean: clean
	-rm -f $(autopcf) $(ptt_sources:.c=.o) $(ptt_sources:.c=.go) $(ptt_object) $(ptt_debug)

