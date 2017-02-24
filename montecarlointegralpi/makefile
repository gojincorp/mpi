EXECS=eyee_monte_carlo_integral_pi
MPICC?=mpicc

all: ${EXECS}

eyee_monte_carlo_integral_pi: eyee_monte_carlo_integral_pi.c
	${MPICC} -o eyee_monte_carlo_integral_pi eyee_monte_carlo_integral_pi.c

clean:
	rm -f ${EXECS}
