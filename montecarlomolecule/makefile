EXECS=eyee_monte_carlo_molecule
MPICC?=mpicc

all: ${EXECS}

eyee_monte_carlo_molecule: eyee_monte_carlo_molecule.c
	${MPICC} -o eyee_monte_carlo_molecule eyee_monte_carlo_molecule.c

clean:
	rm -f ${EXECS}
