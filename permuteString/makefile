EXECS=eyee_permute_string
MPICC?=mpicc

all: ${EXECS}

eyee_permute_string : eyee_permute_string.c
	${MPICC} -o eyee_permute_string eyee_permute_string.c

clean:
	rm -f ${EXECS}
