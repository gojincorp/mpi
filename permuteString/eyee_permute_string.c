#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#define TOTALLETTERS ('z' - '0') 
#define ZERO '0'
#define MAXSTRING 20 

void permute(int * char_set, int skip, int noskip, char * permute_str, int level, int maxLevel, unsigned long * counter);
void permuteNew(int * char_set, unsigned long local_permute_start, unsigned long * local_permute_total, char * permute_str, int level, int maxLevel, unsigned long * counter, unsigned long * permute_counter_arr, unsigned long * permute_counter_branch);

int main(int argc, char ** argv)
{
	MPI_Status mpi_status;
	int		comm_size, comm_rank;
	char		in_str[MAXSTRING];
	int		char_counter[TOTALLETTERS];
	char		* permute_str;
	unsigned long	* permute_counter_arr, * permute_counter_branch;
	int		i, j, temp, str_len;
	unsigned long	permute_count = 0;
	double		timer1, timer2;
	int		total_unique_char;
	int		skiplevel, skip, noskip;
	unsigned long	permutations;
	unsigned long	local_permute_start, local_permute_total;

	// Initialize MPI
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

	while(1)
	{
		// Root process needs to ask the user for a set of up to 20 alphanumeric characters.
		// For the sake of simplicity, I'm forgoing any validation and assuming that the
		// user will input valid data.
		if (0 == comm_rank)
		{
			// Ask for and store user input.
			puts("Please enter up to 20 alphanumeric characters (no special characters...'*' to exit):");
			scanf("%s", in_str);
			str_len = strlen(in_str);

			// Clear out read buffer.
			while(getchar() != '\n')
				continue;

			// Send string to all processes
			for (i = 1; i < comm_size; i++)
				MPI_Send(&in_str, strlen(in_str), MPI_CHAR, i, 0, MPI_COMM_WORLD); 
		}
		// Non-root processes need to listen for message from root...retrieve and store user input.
		else 
		{
			MPI_Probe(0, 0, MPI_COMM_WORLD, &mpi_status);
	
			MPI_Get_count(&mpi_status, MPI_CHAR, &str_len);
			MPI_Recv(&in_str, str_len, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
			in_str[str_len] = '\0';
		}
 
		// Quit application if user enters "*" value.
		if (in_str[0] == '*')
			break;

		// Initialize counters
		permutations = 0;
		for (i = 0; i < TOTALLETTERS; i++)
		{
			char_counter[i] = 0;
		}

		// Tally up count for each character and keep track of how many are unique
		permute_counter_arr = malloc(sizeof(unsigned long) * str_len + 1);
		permute_counter_branch = malloc(sizeof(unsigned long) * str_len + 1);
		total_unique_char = 0;
		for (i = 0; i < str_len; i++)
		{
			// permutations = str_len! (i.e, factorial)
			if (permutations)
				permutations *= (str_len - i);
			else
				permutations = str_len;

			// Unique character found
			if (!char_counter[in_str[i] - ZERO])
				total_unique_char++;	

			// Tally up count for each character
			char_counter[in_str[i] - ZERO]++;		

			// Initialize arrays passed to permuteNew() function
			permute_counter_arr[i] = 0;
			permute_counter_branch[i] = 0;
		}		
		permute_counter_arr[str_len] = 0;
		permute_counter_branch[str_len] = 0;

		// Need to check for repeated characters and divide permutations accordingly
		temp = 1;
		for (i = 0; i < TOTALLETTERS; i++)
		{
			if (char_counter[i] > 1)
			{
				for (j = char_counter[i]; j > 0; j--)
				{
					temp *= j; 
				}	
			}
		}
		permutations /= temp;
		permute_counter_arr[0] = permutations;
		permute_counter_branch[0] = permutations;
		MPI_Barrier(MPI_COMM_WORLD);
		if (0 == comm_rank)
			printf("Calculated total permutations = %lu\n", permutations);
		if (permutations < comm_size)
		{
			local_permute_start = comm_rank < (permutations % comm_size) ? comm_rank : 0;
			local_permute_total = comm_rank < (permutations % comm_size) ? 1 : 0;
		}
		else
		{
			local_permute_start = comm_rank * (permutations/comm_size) + (comm_rank < (permutations%comm_size) ? comm_rank : permutations%comm_size);
			local_permute_total = (permutations/comm_size) + (comm_rank < (permutations%comm_size) ? 1 : 0);
		}
/*
		//local_permute_start = comm_rank * (permutations/comm_size) + (comm_rank < (permutations%comm_size) ? comm_rank : permutations%comm_size);
		if (permutations/comm_size)
			local_permute_start = comm_rank * (permutations/comm_size) + (comm_rank < (permutations%comm_size) ? comm_rank : permutations%comm_size);
		else
			local_permute_start = (comm_rank < (permutations%comm_size) ? comm_rank : 0);
		//local_permute_start = comm_rank * (permutations/comm_size) + (comm_rank < (permutations%comm_size) ? comm_rank : 0);
		if (local_permute_start)
			local_permute_total = local_permute_start + (permutations/comm_size) + (comm_rank < (permutations%comm_size) ? 1 : 0) - 1;
		else
			local_permute_total = 0;
*/
		MPI_Barrier(MPI_COMM_WORLD);
		printf("Rank%d permutations: start/end (%d/%d) str_len=%d\n", comm_rank, local_permute_start, local_permute_total, str_len);

		if (total_unique_char < comm_size)
		{

		}

		// Calculate skip/noskip per process/rank 
		skip = comm_rank * (total_unique_char/comm_size) + (comm_rank < (total_unique_char%comm_size) ? comm_rank : 0);
		noskip = (total_unique_char/comm_size) + (comm_rank < (total_unique_char%comm_size)) ? 1 : 0;
//		printf("Rank%d: skip/noskip (%d/%d)\n", comm_rank, skip, noskip);

		for (i = 0; i < TOTALLETTERS; i++)
		{
			/*
			if (char_counter[i])
				printf("%c : %d\n", ZERO + i, char_counter[i]);
			*/
		}

		permute_count = 0;
		permute_str = malloc(sizeof(char) * str_len + 1);
		permute_str[str_len] = '\0';
		MPI_Barrier(MPI_COMM_WORLD);
		timer1 = -MPI_Wtime();
		/*
		if (noskip)
			permute(char_counter, skip, noskip, permute_str, 0, str_len, &permute_count);
		*/
		permuteNew(char_counter, local_permute_start, &local_permute_total, permute_str, 0, str_len, &permute_count, permute_counter_arr, permute_counter_branch);
		//MPI_Barrier(MPI_COMM_WORLD);
		timer1 += MPI_Wtime();

		MPI_Barrier(MPI_COMM_WORLD);
		printf("Rank#%d:  time=%lf total permutations=%lu\n", comm_rank, timer1, permute_count);

		MPI_Barrier(MPI_COMM_WORLD);

		if (comm_rank == 0)
		{
//			for (i = 0; i < str_len; i++)
//				printf("Final:  Level %i permutations = %lu\n", i, permute_counter_arr[i]);
		}

		free(permute_str);
		free(permute_counter_arr);
		free(permute_counter_branch);
		MPI_Barrier(MPI_COMM_WORLD);
	}

	MPI_Finalize();
	return 0;
}


void permuteNew(int * char_set, unsigned long local_permute_start, unsigned long * local_permute_total, char * permute_str, int level, int maxLevel, unsigned long * counter, unsigned long * permute_counter_arr, unsigned long * permute_counter_branch)
{
	int i, j;
	int comm_rank;
	double timer0;
	MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

if (comm_rank >= 0 && level == 0)
{
	printf("Rank#%d calling permuteNew: level=%d | maxLevel=%lu | local_permute_total=%d\n", comm_rank, level, maxLevel, *local_permute_total);	
}

	if (!(*local_permute_total))
		return;

	if (level == maxLevel)
	{
		(*counter)++;
		(*local_permute_total)--;
if (comm_rank >= 9999)
{
		printf("Permute Rank#%d (%lu):\t%s\n", comm_rank, *counter, permute_str);
		for (i = 0; i <= maxLevel; i++)
			printf("Rank#%d Level %i permutations = %lu\n", comm_rank, i, permute_counter_arr[i]);
}
	}

	for (i = 0; i < TOTALLETTERS; i++)
	{
		if (char_set[i] != 0)
		{
			permute_str[level] = ZERO + i;

			permute_counter_branch[level + 1] += permute_counter_branch[level] * char_set[i] / (maxLevel - level);
			//permute_counter_arr[level + 1] += permute_counter_branch[level] * char_set[i] / (maxLevel - level);
			permute_counter_arr[level + 1] += permute_counter_branch[level + 1];

if (comm_rank >= 9999)
	printf("\tRank#%d:  level=%d, i=%d, char_set[i]=%d, char=%c | %lu/%lu\n", comm_rank, level, i, char_set[i], permute_str[level], local_permute_start, permute_counter_arr[level + 1]);

			//if (local_permute_start <= permute_counter_arr[level + 1] && local_permute_total >= permute_counter_arr[level + 1])
			if (local_permute_start >= permute_counter_arr[level + 1])
			{
				for (j = level + 2; j <= maxLevel; j++)
					permute_counter_arr[j] += permute_counter_branch[level] * char_set[i] / (maxLevel - level);	
			}
			else
			{	 
				char_set[i]--;
				permuteNew(char_set, local_permute_start, local_permute_total, permute_str, level + 1, maxLevel, counter, permute_counter_arr, permute_counter_branch);
				char_set[i]++;
			}

			permute_counter_branch[level + 1] -= permute_counter_branch[level] * char_set[i] / (maxLevel - level);
		}
	} 
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
