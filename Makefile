
CC=clang
CFLAGS=-Wall -g

BINS=memory_shim.so leakcount sctracer


all: $(BINS)

memory_shim.so:  memory_shim.c
	$(CC) $(CFLAGS) -fPIC -shared -o memory_shim.so memory_shim.c -ldl

leakcount:  leakcount.c
	$(CC) $(CFLAGS) -o leakcount leakcount.c

sctracer:  sctracer.c
	$(CC) $(CFLAGS) -o sctracer sctracer.c

clean:
	rm $(BINS)
