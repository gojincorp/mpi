EXECS=parallel_quick_sort
MPICC?=mpicc

all: ${EXECS}

parallel_quick_sort : parallel_quick_sort.c
	${MPICC} -o parallel_quick_sort parallel_quick_sort.c

clean:
	rm -f ${EXECS}
