PROGRAMS := dotprod matmul prodcons

CFLAGS := -Wall -pipe -O3 -pthread

MAKEFLAGS += -r

all: $(PROGRAMS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<
