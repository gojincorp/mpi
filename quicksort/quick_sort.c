#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <time.h>

#define MAXLENGTH 100

void quick_sort(char * user_array, const int low, const int high);
int quick_pivot(char * user_array, int pivot, int low, int high); 
void quick_check(char * user_array, int start, int end, int pivot, int low, int high);

char * my_fgets(char * str, int limit)
{
	char * return_str;
	int i = 0;

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

int main(int argc, char ** argv)
{
	int comm_size, comm_rank;
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
	char user_str[MAXLENGTH];

	while (1 && (comm_rank == 0)) {
		puts("Enter set of up to 100 characters to sort (blank line to quit):");
		if (my_fgets(user_str, MAXLENGTH) == NULL || user_str[0] == '\0')
			break;

		quick_sort(user_str, 0, strlen(user_str) - 1);
		printf("Final sorted array: %s\n", user_str);
	}

	printf("END Rank#%d\n", comm_rank);
	MPI_Finalize();
}

void quick_sort(char * user_array, const int low, const int high)
{
	int i, j;
	int pivot;
	if (low >= high)
		return;

	srand(time(NULL));
	pivot = low + (rand() % (high - low + 1));	

	fputs("SELECT PIVOT POINT:\t", stdout);
	quick_check(user_array, low, high, pivot, low, high);

	pivot = quick_pivot(user_array, pivot, low, high);

	if (low < pivot - 1)
		quick_sort(user_array, low, pivot - 1);
	if (pivot + 1 < high)
		quick_sort(user_array, pivot + 1, high);
}

void quick_check(char * user_array, int start, int end, int pivot, int low, int high)
{
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

	fputs("Swap pivot with high:\t", stdout);
	quick_check(user_array, start, end, pivot, low, high);

	high = pivot - 1;
	
	fputs("Reset high index:\t", stdout);
	quick_check(user_array, start, end, pivot, low, high);
/*	
	while (low < high) {
		// Step #2:  Increment low index until value is greater than pivot
		fputs("Compare low vs pivot:\t", stdout);
		quick_check(user_array, start, end, pivot, low, high);
		if (user_array[low] <= user_array[pivot]) {
			low++;
			
			fputs("Low < pivot...index++:\t", stdout);
			quick_check(user_array, start, end, pivot, low, high);
		}
		// Step #3:  Decrement high index until value is less than or equal to pivot
		fputs("Compare high vs pivot:\t", stdout);
		quick_check(user_array, start, end, pivot, low, high);
		if (user_array[high] > user_array[pivot]) {
			high--;
			
			fputs("high > pivot...index--:\t", stdout);
			quick_check(user_array, start, end, pivot, low, high);
		}
		
		// Step #4:  Swap low with high and repeat step #2 unless low index is greater than or equal to high index
		if (low < high) {
			t_char = user_array[low];
			user_array[low] = user_array[high];
			user_array[high] = t_char;

			fputs("Swap low with high...:\t", stdout);
			quick_check(user_array, start, end, pivot, low, high);
			low++;
			high--;
			fputs("low++ && high--:\t", stdout);
			quick_check(user_array, start, end, pivot, low, high);
		}
	}
*/
	while (low < high) {
		if (user_array[low] <= user_array[pivot]) {
			low ++;
			fputs("Low <= pivot...index++:\t", stdout);
			quick_check(user_array, start, end, pivot, low, high);
		} else if (user_array[high--] <= user_array[pivot]) {
			t_char = user_array[low];
			user_array[low] = user_array[++high];
			user_array[high] = t_char;
			fputs("Swap low with high...:\t", stdout);
			quick_check(user_array, start, end, pivot, low, high);
			low++;
			fputs("low <= pivot...index++:\t", stdout);
			quick_check(user_array, start, end, pivot, low, high);
		} else {
			fputs("high > pivot...index--:\t", stdout);
			quick_check(user_array, start, end, pivot, low, high);
		}
	}
	// Step $5:  Swap pivot with low
	if (user_array[low] > user_array[pivot])
	{
		fputs("low > pivot..........:\t", stdout);
		quick_check(user_array, start, end, pivot, low, high);
		t_char = user_array[low];
		user_array[low] = user_array[pivot];
		user_array[pivot] = t_char; 
		
		t_int = low;
		low = pivot;
		pivot = t_int;
		fputs("Swap low with pivot...:\t", stdout);
		quick_check(user_array, start, end, pivot, low, high);
	} else {
		fputs("End of partition...:\t", stdout);
		quick_check(user_array, start, end, pivot, low, high);
	}

	return pivot;
}
