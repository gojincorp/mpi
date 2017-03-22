#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <time.h>

#define ZERO '0'		// Base character used for normalization 
#define RANGE ('z' - '0')	// Normalized range of random characters

void parallel_quick_sort(char * data_arr, const int start_index, const int end_index, MPI_Comm comm);
int parallel_quick_pivot(char * data_arr, char pivotChar, int low, int high, MPI_Comm comm); 
void parallel_quick_check(char * prefix, char * data_arr, int start_index, int end_index, int pivot, int low, int high, MPI_Comm comm);
void parallel_status(char * prefix, char * data_arr, int start_index, int end_index, char pivotChar, int low, int high, MPI_Comm comm);

void quick_sort(char * user_array, const int low, const int high);
int quick_pivot(char * user_array, int pivot, int low, int high); 
void quick_check(char * user_array, int start, int end, int pivot, int low, int high);

/*
 * @function	my_fgets
 * @abstract	fgets wrapper function to help clear stdin buffer and discard newline character
 * @param	char * str: used to store input string
 * @param	int limit: maximum number of characters to read including implicit '\0' terminator
 * @return	char *: pointer to input string parameter or null pointer (if end-of-file found).
 * @author	Eric Yee
 */
char * my_fgets(char * str, int limit)
{
	char	* return_str;
	int	i = 0;

	return_str = fgets(str, limit, stdin);
	if (return_str) {
		while (str[i] != '\0' && str[i] != '\n')
			i++;
		if (str[i] == '\n')
			str[i] = '\0';
		else 
			while (getchar() != '\n')
				continue;
	}

	return return_str;
}

/*
 * @function	rand_string
 * @abstract	generates a random string of specified length
 * @param	char *:
 *			randStr:  random string result.
 * @param	int:
 *			size: size of random string to generate
 * @return	void.	
 * @author:	Eric Yee
 */
char * rand_string(char *rand_str, int size)
{
	int	i;

	srand(time(NULL));
	for (i = 0; i < size; i++) {
		rand_str[i] = ZERO + (rand() % (RANGE + 1));	
	}
	rand_str[size] = '\0';
 
	return rand_str;
}

/*
 * @function	main
 * @abstract	This program is a parallel quicksort exercise
 * @param	int argc: number of arguments
 * @param	char ** argv: array of string arguments.
 * @return	int:  failure/success return code.
 * @author	Eric Yee
 */
int main(int argc, char ** argv)
{
	int	comm_size, comm_rank;
	int	rand_str_len;
	char	* rand_str;
	int	* scatter_cnt, * scatter_disp; 
	int	i;
	int	quotient, remainder;
	char	* recv_buff;

	// Initialize MPI
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

	// Allocate memory for scatter parameters
	scatter_cnt = malloc(sizeof(int) * comm_size);	
	scatter_disp = malloc(sizeof(int) * comm_size);	

	while (1) {
		// Request and broadcast number of random characters to generate.
		MPI_Barrier(MPI_COMM_WORLD);
		if (comm_rank == 0) {
			puts("How many characters would you like us to sort (0 to quit)?");
			scanf("%d", &rand_str_len);
		}
		MPI_Bcast(&rand_str_len, 1, MPI_INT, 0, MPI_COMM_WORLD);

		// Exit application if user enters 0 (zero).
		if (!rand_str_len)
			break;

		// Generate random characters
		if (comm_rank == 0) {
			rand_str = (char *) malloc(sizeof(char) * (rand_str_len + 1));
			rand_string(rand_str, rand_str_len);
			printf("The random %d characters are:  %s\n", rand_str_len, rand_str); 
		}

		// Calculate scatter count and displacement values
		quotient = rand_str_len/comm_size;
		remainder = rand_str_len%comm_size;
		for (i = 0; i < comm_size; i++) {
			scatter_cnt[i] = quotient + ((i < remainder) ? 1 : 0);
			scatter_disp[i] = (i == 0) ? 0 : (scatter_disp[i-1] + scatter_cnt[i - 1]);
		}

		// Scatter random characters
		recv_buff = malloc(sizeof(char) * (scatter_cnt[comm_rank] + 1));
		MPI_Scatterv(rand_str, scatter_cnt, scatter_disp, MPI_CHAR, recv_buff, scatter_cnt[comm_rank], MPI_CHAR, 0, MPI_COMM_WORLD); 
		recv_buff[scatter_cnt[comm_rank]] = '\0';

		MPI_Barrier(MPI_COMM_WORLD);
		printf("Rank #%-2d: %2d | %s\n", comm_rank, scatter_cnt[comm_rank], recv_buff);

		// Proceed with parallelized quick sort
		MPI_Barrier(MPI_COMM_WORLD);
		parallel_quick_sort(recv_buff, 0, scatter_cnt[comm_rank] - 1, MPI_COMM_WORLD);
		//printf("Final sorted array: %s\n", rand_str);
		if (comm_rank == 0)
			free(rand_str);
		free(recv_buff);
	}

	printf("END Rank#%d\n", comm_rank);
	MPI_Finalize();

	// Free memory
	free(scatter_cnt);
	free(scatter_disp);
}

/*
 * @function	parallel_quick_sort
 * @abstract	Parallelized quick sort algorithm
 * @param	char *
 *			data_arr:
 * @param	const int
 *			start_index:
 * @param	const int
 *			end_index:
 * @param	MPI_Comm
 *			comm:
 * @return	void
 * @author	Eric Yee
 */
void parallel_quick_sort(char * data_arr, const int start_index, const int end_index, MPI_Comm comm)
{
	int		world_size, world_rank, comm_size, comm_rank;
	MPI_Status	status;
	MPI_Comm	sub_comm;
	int		i, j;
	int		pivot, rand_rank;
	char		pivotChar;
	int		rank_pivot, exchange_count;
	char		* new_arr;
	int		index_cnt = end_index - start_index + 1;
	int		* index_cnt_arr;
	int		*gather_cnt, *gather_disp;
	char 		*recv_buff;

	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(comm , &comm_size);
	MPI_Comm_rank(comm, &comm_rank);

	if (comm_size <= 1) {
		// Gather index_cnt from all processes.
		index_cnt_arr = malloc(sizeof(int) * world_size);
		MPI_Gather(&index_cnt, 1, MPI_INT, index_cnt_arr, 1, MPI_INT, 0, MPI_COMM_WORLD);
	
		//quick_sort(data_array, const int low, const int high);
		MPI_Barrier(MPI_COMM_WORLD);
		quick_sort(data_arr, 0, strlen(data_arr) - 1);
		MPI_Barrier(MPI_COMM_WORLD);

		// Allocate memory for scatter parameters
		gather_cnt = malloc(sizeof(int) * world_size);	
		gather_disp = malloc(sizeof(int) * world_size);	
		for (i = 0; i < world_size; i++) {
			gather_cnt[i] = index_cnt_arr[i];
			gather_disp[i] = (i == 0) ? 0 : (gather_disp[i-1] + gather_cnt[i - 1]);
		}

		printf("Final Rank #%-2d (%-2d/%-2d): %d | %s | %d\n", world_rank, comm_rank, comm_size, index_cnt, data_arr, gather_disp[world_size - 1]);
		if (world_rank == 0)
			recv_buff = malloc(sizeof(char) * (gather_disp[world_size - 1] + gather_cnt[world_size - 1] + 1));
		else
			recv_buff = NULL;
		MPI_Gatherv(data_arr, index_cnt, MPI_CHAR, recv_buff, gather_cnt, gather_disp, MPI_CHAR, 0, MPI_COMM_WORLD);
		if (world_rank == 0)
			recv_buff[(gather_disp[world_size - 1] + gather_cnt[world_size - 1])] = '\0';

		MPI_Barrier(MPI_COMM_WORLD);
		if (world_rank == 0)
			printf("Final result:  %s\n", recv_buff); 
		free(index_cnt_arr);
		free(gather_cnt);
		free(gather_disp);
		if (world_rank = 0)
			free(recv_buff);
		return;
	}

	// Gather index_cnt from all processes.
	index_cnt_arr = malloc(sizeof(int) * comm_size);
	MPI_Gather(&index_cnt, 1, MPI_INT, index_cnt_arr, 1, MPI_INT, 0, comm);
	

	// Root process selects a starting random rank.
	if (comm_rank == 0) {
		srand(time(NULL));
		i = comm_size;
		// Choose a random rank
		rand_rank = rand() % comm_size;	
		while (!index_cnt_arr[rand_rank] && i--) {
			rand_rank = (rand_rank + 1) % comm_size;	
		}
		if (i < 0) { // Seems that there is nothing to sort...
			rand_rank = -1;
		}
	}
	MPI_Bcast(&rand_rank, 1, MPI_INT, 0, comm);
	if (rand_rank == -1) {
		//printf("Final Rank #%-2d (%-2d/%-2d): %s\n", world_rank, comm_rank, comm_size, data_arr);
		free(index_cnt_arr);
		MPI_Comm_split(comm, comm_rank, comm_rank, &sub_comm);
		parallel_quick_sort(data_arr, 0, -1, sub_comm);
		MPI_Comm_free(&sub_comm);
		return;
	}

	if (comm_rank == rand_rank) {
		// Choose a random pivot
		pivot = start_index + (rand() % (end_index - start_index + 1));	
		pivotChar = data_arr[pivot];
		printf("Select random pivot:  Rank#%d | pivot=%d | char=%c\n", comm_rank, pivot, pivotChar);
	}
	// Broadcast pivot point within group
	MPI_Bcast(&pivotChar, 1, MPI_CHAR, rand_rank, comm);

	if (comm_rank == 0)
		printf("pivotChar=%c\n", pivotChar);

	// Debug 
	parallel_status("SELECT PIVOT CHAR:", data_arr, start_index, end_index, pivotChar, start_index, end_index, comm);

	// Partition around pivot point
	if (index_cnt)
		pivot = parallel_quick_pivot(data_arr, pivotChar, start_index, end_index, comm);

	MPI_Barrier(comm);
	// Exchange low/high partitions
	rank_pivot = comm_size/2;
	if (comm_rank < rank_pivot) {
		//printf("Rank#%d: %s | pivot=%d | send=%d | to=%d\n", comm_rank, data_arr,  pivot, (end_index - pivot), (rank_pivot + comm_rank));
		if (index_cnt)
			MPI_Send((data_arr + pivot + 1), (end_index - pivot), MPI_CHAR, (rank_pivot + comm_rank), 0, comm);
		else
			MPI_Send(data_arr, 0, MPI_CHAR, (rank_pivot + comm_rank), 0, comm);
		//MPI_Send(data_arr, 1, MPI_CHAR, (rank_pivot + comm_rank), 0, comm);
		MPI_Probe((comm_rank + rank_pivot), 0, comm, &status);
		MPI_Get_count(&status, MPI_CHAR, &exchange_count);
		if (index_cnt) 
			new_arr = malloc(sizeof(char) * ((pivot + 1) + exchange_count + 1));
		else
			new_arr = malloc(sizeof(char) * (exchange_count + 1));
		MPI_Recv(new_arr, exchange_count, MPI_CHAR, (comm_rank + rank_pivot), 0, comm, MPI_STATUS_IGNORE);
		if (index_cnt)
			memcpy(new_arr + exchange_count, data_arr, (pivot + 1));
		if (index_cnt)
			new_arr[((pivot + 1) + exchange_count)] = '\0';
		else
			new_arr[exchange_count] = '\0';
		//printf("Rank#%d: %s | pivot=%d | recv=%d | new_arr=%s\n", comm_rank, data_arr, pivot, exchange_count, new_arr);
	} else {
		//printf("Rank#%d: %s | pivot=%d | send=%d | to=%d\n", comm_rank, data_arr,  pivot, (pivot + 1), (comm_rank - rank_pivot));
		MPI_Probe((comm_rank - rank_pivot), 0, comm, &status);
		MPI_Get_count(&status, MPI_CHAR, &exchange_count);
		if (index_cnt)
			new_arr = malloc(sizeof(char) * ((end_index - pivot) + exchange_count + 1));
		else
			new_arr = malloc(sizeof(char) * (exchange_count + 1));
		MPI_Recv(new_arr, exchange_count, MPI_CHAR, (comm_rank - rank_pivot), 0, comm, MPI_STATUS_IGNORE);
		if (index_cnt)
			memcpy(new_arr + exchange_count, data_arr + (pivot + 1), (end_index - pivot));
		if (index_cnt)
			new_arr[((end_index - pivot) + exchange_count)] = '\0';
		else
			new_arr[exchange_count] = '\0';
		//printf("Rank#%d: %s | pivot=%d | recv=%d | new_arr=%s\n", comm_rank, data_arr, pivot, exchange_count, new_arr);
		if (index_cnt)
			MPI_Send(data_arr, (pivot + 1), MPI_CHAR, (comm_rank - rank_pivot), 0, comm);
		else
			MPI_Send(data_arr, 0, MPI_CHAR, (comm_rank - rank_pivot), 0, comm);
	}

	MPI_Barrier(comm);
	printf("Rank #%-2d (%-2d/%-2d): %2zu | %s\n", world_rank, comm_rank, comm_size, sizeof(new_arr), new_arr);

	if (comm_size > 1) {
		MPI_Comm_split(comm, (comm_rank < rank_pivot) ? 0 : 1, comm_rank, &sub_comm);
		parallel_quick_sort(new_arr, 0, strlen(new_arr) - 1, sub_comm);
		MPI_Comm_free(&sub_comm);
	}
	free(index_cnt_arr);
	free(new_arr);
}

void parallel_status(char * prefix, char * data_arr, int start_index, int end_index, char pivotChar, int low, int high, MPI_Comm comm)
{
	return;
	int world_size, world_rank, comm_size, comm_rank;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(comm , &comm_size);
	MPI_Comm_rank(comm, &comm_rank);
	int i, j;
	char * tempStr = malloc(sizeof(int) * ((end_index - start_index) + 6));
	for (i = start_index, j = 0; i <= end_index; i++) {
		if (i == low)
			tempStr[i + j++] = '['; 
		if (i == high)
			tempStr[i + j++] = '{'; 

		tempStr[i + j] = data_arr[i];

		if (i == high)
			tempStr[1 + i + j++] = '}'; 
		if (i == low)
			tempStr[1 + i + j++] = ']'; 
	}	
	tempStr[(end_index - start_index) + 5] = '\0';
	printf("%-30s #%-2d/%-2d(%-2d/%-2d)\t%s | %c\n", prefix, comm_rank, comm_size, world_rank, world_size, tempStr, pivotChar);

	free(tempStr);
}

void parallel_quick_check(char * prefix, char * data_arr, int start_index, int end_index, int pivot, int low, int high, MPI_Comm comm)
{
	return;
	int world_size, world_rank, comm_size, comm_rank;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(comm , &comm_size);
	MPI_Comm_rank(comm, &comm_rank);
	int i, j;
	char * tempStr = malloc(sizeof(int) * ((end_index - start_index) + 8));
	for (i = start_index, j = 0; i <= end_index; i++) {
		if (i == pivot)
			tempStr[i + j++] = '('; 
		if (i == low)
			tempStr[i + j++] = '['; 
		if (i == high)
			tempStr[i + j++] = '{'; 

		tempStr[i + j] = data_arr[i];

		if (i == high)
			tempStr[1 + i + j++] = '}'; 
		if (i == low)
			tempStr[1 + i + j++] = ']'; 
		if (i == pivot)
			tempStr[1 + i + j++] = ')'; 
	}	
	tempStr[(end_index - start_index) + 7] = '\0';
	printf("%-30s #%-2d/%-2d(%-2d/%-2d)\t%s\n", prefix, comm_rank, comm_size, world_rank, world_size, tempStr);

	free(tempStr);
}

int parallel_quick_pivot(char * data_arr, char pivotChar, int low, int high, MPI_Comm comm)
{
	char t_char;
	int t_int;
	int start = low, end = high;

	while (low < high) {
		if (data_arr[low] <= pivotChar) {
			low ++;
			parallel_quick_check("Low <= pivot...index++:", data_arr, start, end, pivotChar, low, high, comm);
		} else if (data_arr[high--] <= pivotChar) {
			t_char = data_arr[low];
			data_arr[low] = data_arr[++high];
			data_arr[high] = t_char;
			parallel_quick_check("Swap low with high...:", data_arr, start, end, pivotChar, low, high, comm);
			low++;
			parallel_quick_check("low <= pivot...index++:", data_arr, start, end, pivotChar, low, high, comm);
		} else {
			parallel_quick_check("high > pivot...index--:", data_arr, start, end, pivotChar, low, high, comm);
		}
	}
	// Step $5:  Swap pivot with low
	if (data_arr[low] > pivotChar)
	{
		return low - 1;
	} else {
		return low;
	}
}


void quick_sort(char * user_array, const int low, const int high)
{
	int i, j;
	int pivot;
	if (low >= high)
		return;

	srand(time(NULL));
	pivot = low + (rand() % (high - low + 1));	

	//fputs("SELECT PIVOT POINT:\t", stdout);
	quick_check(user_array, low, high, pivot, low, high);

	pivot = quick_pivot(user_array, pivot, low, high);

	if (low < pivot - 1)
		quick_sort(user_array, low, pivot - 1);
	if (pivot + 1 < high)
		quick_sort(user_array, pivot + 1, high);
}

void quick_check(char * user_array, int start, int end, int pivot, int low, int high)
{
return;
	int i;

	for (i = start; i <= end; i++) {
		if (i == pivot)
			fputs("(", stdout);
		if (i == low)
			fputs("[", stdout);
		if (i == high)
			fputs("{", stdout);

		putchar(user_array[i]);

		if (i == high)
			fputs("}", stdout);
		if (i == low)
			fputs("]", stdout);
		if (i == pivot)
			fputs(")", stdout);
	} 

	puts("");
}

int quick_pivot(char * user_array, int pivot, int low, int high)
{
	char t_char;
	int t_int;
	int start = low, end = high;

	// Step #1:  Swap pivot char with high char 
	t_char = user_array[high];
	user_array[high] = user_array[pivot];
	user_array[pivot] = t_char;
	t_int = high;
	high = pivot;
	pivot = t_int;

	//fputs("Swap pivot with high:\t", stdout);
	quick_check(user_array, start, end, pivot, low, high);

	high = pivot - 1;
	
	//fputs("Reset high index:\t", stdout);
	quick_check(user_array, start, end, pivot, low, high);
	while (low < high) {
		if (user_array[low] <= user_array[pivot]) {
			low ++;
			//fputs("Low <= pivot...index++:\t", stdout);
			quick_check(user_array, start, end, pivot, low, high);
		} else if (user_array[high--] <= user_array[pivot]) {
			t_char = user_array[low];
			user_array[low] = user_array[++high];
			user_array[high] = t_char;
			//fputs("Swap low with high...:\t", stdout);
			quick_check(user_array, start, end, pivot, low, high);
			low++;
			//fputs("low <= pivot...index++:\t", stdout);
			quick_check(user_array, start, end, pivot, low, high);
		} else {
			//fputs("high > pivot...index--:\t", stdout);
			quick_check(user_array, start, end, pivot, low, high);
		}
	}
	// Step $5:  Swap pivot with low
	if (user_array[low] > user_array[pivot])
	{
		//fputs("low > pivot..........:\t", stdout);
		quick_check(user_array, start, end, pivot, low, high);
		t_char = user_array[low];
		user_array[low] = user_array[pivot];
		user_array[pivot] = t_char; 
		
		t_int = low;
		low = pivot;
		pivot = t_int;
		//fputs("Swap low with pivot...:\t", stdout);
		quick_check(user_array, start, end, pivot, low, high);
	} else {
		//fputs("End of partition...:\t", stdout);
		quick_check(user_array, start, end, pivot, low, high);
	}

	return pivot;
}
