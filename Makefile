CUPID_LIBS=

all: clean cupidfm

cupidfm: src/*.c
	$(CC) -o $@ $< --std=c2x $(CFLAGS) $(LDFLAGS) $(CUPID_LIBS) $(LIBS) $(LD_LIBS)


.PHONY: clean

clean:
	rm -f cupidfm *.o



