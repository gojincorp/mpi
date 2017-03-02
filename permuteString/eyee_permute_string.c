#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#define TOTALLETTERS ('z' - '0') 
#define ZERO '0'
#define MAXSTRING 20 

void permute(int * char_set, int skip, int noskip, char * permute_str, int level, int maxLevel, unsigned long * counter);

int main(int argc, char ** argv)
{
	int		comm_size, comm_rank;
	char		in_str[MAXSTRING];
	int		char_counter[TOTALLETTERS];
	char		* permute_str;
	int		i, str_len;
	unsigned long	permute_count = 0;
	double		timer1, timer2;
	int		unique_count;
	int		skip, noskip;

	MPI_Status mpi_status;
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

	while(1)
	{
		if (0 == comm_rank)
		{
			puts("Please enter up to 20 alphanumeric characters (no special characters...'*' to exit):");
			scanf("%s", in_str);
			while(getchar() != '\n')
				continue;

			// Send string
			for (i = 1; i < comm_size; i++)
			MPI_Send(&in_str, strlen(in_str), MPI_CHAR, i, 0, MPI_COMM_WORLD); 
		}
		else 
		{
			MPI_Probe(0, 0, MPI_COMM_WORLD, &mpi_status);
	
			MPI_Get_count(&mpi_status, MPI_CHAR, &str_len);
			MPI_Recv(&in_str, str_len, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
			in_str[str_len] = '\0';
		}
 
		if (in_str[0] == '*')
			break;

		unique_count = 0;
		str_len = strlen(in_str);

		permute_count = 0;
		permute_str = malloc(sizeof(char) * str_len + 1);
		permute_str[str_len] = '\0';

		// Initialize char_counter[]
		for (i = 0; i < TOTALLETTERS; i++)
		{
			char_counter[i] = 0;
		}

		for (i = 0; i < str_len; i++)
		{
			if (!char_counter[in_str[i] - ZERO])
				unique_count++;	

			char_counter[in_str[i] - ZERO]++;		
		}		

		// Calculate skip/noskip per process/rank 
		skip = comm_rank * (unique_count/comm_size) + (comm_rank < (unique_count%comm_size)) ? comm_rank : 0;
		noskip = (unique_count/comm_size) + (comm_rank < (unique_count%comm_size)) ? 1 : 0;
		printf("Rank%d: skip/noskip (%d/%d)\n", comm_rank, skip, noskip);

		for (i = 0; i < TOTALLETTERS; i++)
		{
			/*
			if (char_counter[i])
				printf("%c : %d\n", ZERO + i, char_counter[i]);
			*/
		}

		MPI_Barrier(MPI_COMM_WORLD);
		timer1 = -MPI_Wtime();
		if (noskip)
			permute(char_counter, skip, noskip, permute_str, 0, str_len, &permute_count);
		//MPI_Barrier(MPI_COMM_WORLD);
		timer1 += MPI_Wtime();

		printf("Rank#%d:  time=%lf total permutations=%lu\n", comm_rank, timer1, permute_count);
		free(permute_str);
		MPI_Barrier(MPI_COMM_WORLD);
	}

	MPI_Finalize();
	return 0;
}


void permute(int * char_set, int skip, int noskip, char * permute_str, int level, int maxLevel, unsigned long * counter)
{
	int i;
	int comm_rank;
	double timer0;
	MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

	if (level == maxLevel)
	{
		(*counter)++;
		//printf("Permute Rank#%d (%lu):\t%s\n", comm_rank, *counter, permute_str);
	}
	
	for (i = 0; i < TOTALLETTERS; i++)
	{
		if (char_set[i] != 0)
		{
			if (!level && skip-- > 0)
				continue;

			if (level || noskip-- > 0)
			{
				permute_str[level] = ZERO + i;
				char_set[i]--;

				permute(char_set, 0, 0, permute_str, level + 1, maxLevel, counter);
				char_set[i]++;
			}
		}
	} 
}	
