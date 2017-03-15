EXECS=quick_sort
MPICC?=mpicc

all: ${EXECS}

quick_sort : quick_sort.c
	${MPICC} -o quick_sort quick_sort.c

clean:
	rm -f ${EXECS}
