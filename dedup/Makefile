PTT_PATH := ../tracelib
PROGRAMS := dedup

CFLAGS     := -O3 -fomit-frame-pointer -std=c99
CFLAGS_DBG := -O0 -g -std=c99

dedup_SOURCES := binheap.c \
                 binheap.h \
                 config.h \
                 debug.h \
                 decoder.c \
                 decoder.h \
                 dedup.c \
                 dedupdef.h \
                 encoder.c \
                 encoder.h \
                 hashtable.c \
                 hashtable.h \
                 hashtable_private.h \
                 queue.c \
                 queue.h \
                 rabin.c \
                 rabin.h \
                 tree.c \
                 tree.h \
                 util.c \
                 util.h

dedup_PCF := events.pcf
dedup_LIBS := z ssl

include $(PTT_PATH)/rules.mk
