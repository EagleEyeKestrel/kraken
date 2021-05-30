CC=g++

all: kraken

kraken: main.o cache.o memory.o co_filter.o
	$(CC) -o $@ $^

main.o: cache.h reader.h

co_filter.o: cache.h co_filter.h

cache.o: cache.h def.h co_filter.h

memory.o: memory.h

.PHONY: clean

clean:
	rm -rf sim *.o
