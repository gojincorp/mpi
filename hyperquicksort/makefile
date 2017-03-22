EXECS=hyper_quick_sort
MPICC?=mpicc

all: ${EXECS}

hyper_quick_sort : hyper_quick_sort.c
	${MPICC} -o hyper_quick_sort hyper_quick_sort.c

clean:
	rm -f ${EXECS}
