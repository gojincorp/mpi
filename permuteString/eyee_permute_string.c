/*
 * Author:  Eric Yee
 * Copyright 2017 www.ideasbeyond.com
 *
 * This is an exercise on how one might go about parallelizing the task
 * of creating a lexically sorted permutation of a user supplied set of
 * alphanumeric characters (case sensitive and including numbers). 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#define MAXCHARSET ('z' - '0') // Using a subset of the standard ASCII character set 
#define ZERO '0'
#define MAXSTRING 20 

void permute(int * char_set, unsigned long permute_start, unsigned long * permute_total, char * permute_str, int level,
		int maxLevel, unsigned long * counter, unsigned long * permute_counter_arr, unsigned long * permute_counter_branch);

int main(int argc, char ** argv)
{
	MPI_Status	mpi_status;
	int		comm_size, comm_rank;
	char		in_str[MAXSTRING];
	int		char_counter1[MAXCHARSET], char_counter2[MAXCHARSET];
	char		* permute_str;
	unsigned long	* permute_counter_arr1, * permute_counter_branch1, * permute_counter_arr2, * permute_counter_branch2;
	int		i, j, temp, str_len;
	unsigned long	permute_count = 0;
	double		timer1, timer2;
	int		total_unique_char;
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
		for (i = 0; i < MAXCHARSET; i++)
		{
			char_counter1[i] = 0;
			char_counter2[i] = 0;
		}

		// Tally up count for each character and keep track of how many are unique
		permute_counter_arr1 = malloc(sizeof(unsigned long) * str_len + 1);
		permute_counter_branch1 = malloc(sizeof(unsigned long) * str_len + 1);
		permute_counter_arr2 = malloc(sizeof(unsigned long) * str_len + 1);
		permute_counter_branch2 = malloc(sizeof(unsigned long) * str_len + 1);
		total_unique_char = 0;
		for (i = 0; i < str_len; i++)
		{
			// permutations = str_len! (i.e, factorial)
			if (permutations)
				permutations *= (str_len - i);
			else
				permutations = str_len;

			// Unique character found
			if (!char_counter1[in_str[i] - ZERO])
				total_unique_char++;	

			// Tally up count for each character
			char_counter1[in_str[i] - ZERO]++;		
			char_counter2[in_str[i] - ZERO]++;		

			// Initialize arrays passed to permute() function
			permute_counter_arr1[i] = 0;
			permute_counter_branch1[i] = 0;
			permute_counter_arr2[i] = 0;
			permute_counter_branch2[i] = 0;
		}		
		permute_counter_arr1[str_len] = 0;
		permute_counter_branch1[str_len] = 0;
		permute_counter_arr2[str_len] = 0;
		permute_counter_branch2[str_len] = 0;

		// Need to check for repeated characters and divide permutations accordingly
		temp = 1;
		for (i = 0; i < MAXCHARSET; i++)
		{
			if (char_counter1[i] > 1)
			{
				for (j = char_counter1[i]; j > 0; j--)
				{
					temp *= j; 
				}	
			}
		}
		permutations /= temp;
		permute_counter_arr1[0] = permutations;
		permute_counter_branch1[0] = permutations;
		permute_counter_arr2[0] = permutations;
		permute_counter_branch2[0] = permutations;
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

		MPI_Barrier(MPI_COMM_WORLD);
		printf("Rank%d permutations: start/end (%d/%d) str_len=%d\n", comm_rank, local_permute_start, local_permute_total, str_len);

		if (total_unique_char < comm_size)
		{

		}

		permute_count = 0;
		permute_str = malloc(sizeof(char) * str_len + 1);
		permute_str[str_len] = '\0';

		MPI_Barrier(MPI_COMM_WORLD);
		if (comm_rank == 0)
		{
			timer2 = -MPI_Wtime();
			unsigned long global_permute_start = 0;
			unsigned long global_permute_total = permutations;
			permute(char_counter2, global_permute_start, &global_permute_total, permute_str, 0, str_len, &permute_count, permute_counter_arr2, permute_counter_branch2);
			timer2 += MPI_Wtime();
			printf("Non-parallel Rank#%d:  time=%lf total permutations=%lu\n", comm_rank, timer2, permute_count);
		}

		permute_count = 0;

		MPI_Barrier(MPI_COMM_WORLD);
		timer1 = -MPI_Wtime();
		permute(char_counter1, local_permute_start, &local_permute_total, permute_str, 0, str_len, &permute_count, permute_counter_arr1, permute_counter_branch1);
		timer1 += MPI_Wtime();

		MPI_Barrier(MPI_COMM_WORLD);
		printf("Rank#%d:  time=%lf total permutations=%lu\n", comm_rank, timer1, permute_count);

		free(permute_str);
		free(permute_counter_arr1);
		free(permute_counter_branch1);
		free(permute_counter_arr2);
		free(permute_counter_branch2);
		MPI_Barrier(MPI_COMM_WORLD);
	}

	MPI_Finalize();
	return 0;
}


void permute(int * char_set, unsigned long local_permute_start, unsigned long * local_permute_total, char * permute_str, int level, int maxLevel, unsigned long * counter, unsigned long * permute_counter_arr, unsigned long * permute_counter_branch)
{
	int i, j;
	int comm_rank;
	double timer0;
	MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

if (comm_rank >= 0 && level == 0)
{
	printf("Rank#%d calling permute: level=%d | maxLevel=%lu | local_permute_total=%d\n", comm_rank, level, maxLevel, *local_permute_total);	
}

	if (!(*local_permute_total))
		return;

	if (level == maxLevel)
	{
		(*counter)++;
		(*local_permute_total)--;
		//printf("Permute Rank#%d (%lu):\t%s\n", comm_rank, *counter, permute_str);
	}

	for (i = 0; i < MAXCHARSET; i++)
	{
		if (char_set[i] != 0)
		{
			permute_str[level] = ZERO + i;

			permute_counter_branch[level + 1] += permute_counter_branch[level] * char_set[i] / (maxLevel - level);
			permute_counter_arr[level + 1] += permute_counter_branch[level + 1];

			if (local_permute_start >= permute_counter_arr[level + 1])
			{
				for (j = level + 2; j <= maxLevel; j++)
					permute_counter_arr[j] += permute_counter_branch[level] * char_set[i] / (maxLevel - level);	
			}
			else
			{	 
				char_set[i]--;
				permute(char_set, local_permute_start, local_permute_total, permute_str, level + 1, maxLevel, counter, permute_counter_arr, permute_counter_branch);
				char_set[i]++;
			}

			permute_counter_branch[level + 1] -= permute_counter_branch[level] * char_set[i] / (maxLevel - level);
		}
	} 
}	
