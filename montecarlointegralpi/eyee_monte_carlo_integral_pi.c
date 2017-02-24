#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

/*
 * Main function.  Prompts user for a positive integer and proceeds to run a 
 * Monte-Carlo stochastic integration to estimate the value of PI.
 *
 * @param int 		argc: count of parameters
 * @param char **	argv: array of parameters
 * @return int		*: return 0 on success
 */
int main(int argc, char ** argv)
{
	int n, comm_rank, comm_size, i;
	double PI = 3.141592653589793238462643;
	double l_pi, g_pi, h, sum, x;

	// Initialize MPI
	MPI_Init(NULL,NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
	
	while (1)
	{
		// Prompt user for a positive integer value
		if (0 == comm_rank)
		{
			puts("Enter n (or an integer < 1 to exit):");
			scanf("%d", &n);
			while (getchar() != '\n')
				continue;
		}

		// Broadcast n to all processes
		MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

		if (n < 1)
		{ // Exit application
			break;
		}
		else
		{ // Run Monte-Carlo method to estimate value of PI
			h = 1.0 / (double)n;
			sum = 0.0;
			for (i = comm_rank + 1; i <= n; i += comm_size)
			{
				x = h * ((double)i - 0.5);
				sum += (4.0 / (1.0 + x * x));
			}
			l_pi = h * sum;

			// Add up "sum" from all process
			MPI_Reduce(&l_pi, &g_pi, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

			if (0 == comm_rank)
			{
				printf("Integral PI = %f | Real PI = %f\n", g_pi, PI);
			}
		}
	}

	MPI_Finalize();
	
	return 0;
}
