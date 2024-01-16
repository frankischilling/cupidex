CUPID_LIBS=-Isrc
CUPID_FLAGS=--std=c2x

all: clean cupidfm

cupidfm: src/*.c src/*.h
	$(CC) -o $@ src/*.c $(CUPID_FLAGS) $(CFLAGS) $(LDFLAGS) $(CUPID_LIBS) $(LIBS) $(LD_LIBS)


.PHONY: clean

clean:
	rm -f cupidfm *.o



