CC := ~/mpi-install/bin/mpicc

OPTS := -std=gnu99 -O1

psrs : psrs.o phases.o
	$(CC) -o $@ $^

%.o : %.c
	$(CC) $(OPTS) -c -o $@ $<

clean :
	rm -rf *.o psrs

psrs.o : psrs.c phases.h
phases.o : phases.c phases.h
