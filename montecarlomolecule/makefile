EXECS=eyee_monte_carlo_pi
MPICC?=mpicc

all: ${EXECS}

eyee_monte_carlo_pi: eyee_monte_carlo_pi.c
	${MPICC} -o eyee_monte_carlo_pi eyee_monte_carlo_pi.c

clean:
	rm -f ${EXECS}
