#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <time.h>
#include <float.h>

#define DIMENSIONS 3 // X, Y and Z for molecular analysis
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/*
 * Get random number between min and max
 *
 * @param double min:  minimum double value
 * @param double max:  maximum double value
 * @return double result:  random double value
 */
double get_rand(double min, double max)
{
	double rand_num = rand()/(double)RAND_MAX;
	return (rand_num * (max - min)) + min;
}

/*
 * Calculate the distance squared between two points 
 *
 * @param double[] pointA: array containing XYZ values 
 * @param double[] pointB: array containing XYZ values 
 * @return double result: distance squared between two points 
 */
double distanceSquared(double pointA[DIMENSIONS], double pointB[DIMENSIONS])
{
	double result = 0, diff;
	int i;
	for (i = 0; i < DIMENSIONS; i++)
	{
		diff = pointA[i] - pointB[i];
		result += diff * diff;
	} 

	return result;
} 

typedef struct mystruct
{
	double radius;
	double x;
	double y;
	double z;
} sphere;

typedef struct
{
	double x_min;
	double x_max;
	double y_min;
	double y_max;
	double z_min;
	double z_max;
} boundingBox;

int main(int argc, char ** arcv)
{
	MPI_Status status;
	int sphere_count = 0, sample_count;
	int comm_size, comm_rank;
	int i, j;
	int scatter_sum = 0;
	int * scatter_count_arr, * scatter_disp;
	sphere * spheres, * recv_spheres;
	boundingBox bb = {DBL_MAX,DBL_MIN,DBL_MAX,DBL_MIN,DBL_MAX,DBL_MIN};

	// Local variables
	int scatter_count; 

	MPI_Init(NULL,NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

	int my_count = 4;
	int my_blocklengths[] = {1, 1, 1, 1};
	MPI_Aint my_displacements[] = {offsetof(struct mystruct, radius), offsetof(struct mystruct, x), offsetof(struct mystruct, y), offsetof(struct mystruct, z)};
	MPI_Datatype my_datatypes[] = {MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE};
	MPI_Datatype tmp_type, my_mpi_type;
	MPI_Aint lb, extent;

	MPI_Type_create_struct(my_count, my_blocklengths, my_displacements, my_datatypes, &tmp_type);
	MPI_Type_get_extent( tmp_type, &lb, &extent);
	MPI_Type_create_resized(tmp_type, lb, extent, &my_mpi_type);
	MPI_Type_commit(&my_mpi_type);

	srand(time(NULL) * (comm_rank + 1));

	if (0 == comm_rank)
	{
		puts("Please enter the number of spheres in your molecule:");
		scanf("%d", &sphere_count);
		while(getchar() != '\n')
			continue;
		puts("Please enter the number of samples you'd like to test:");
		scanf("%d", &sample_count);
		while(getchar() != '\n')
			continue;

		scatter_count_arr = malloc(sizeof(int) * comm_size);
		scatter_disp = malloc(sizeof(int) * comm_size);
		for (i = 0; i < comm_size; i++)
		{
			scatter_count_arr[i] = sphere_count/comm_size;
		}
		for (i = 0; i < (sphere_count % comm_size); i++)
		{
			scatter_count_arr[i]++;
		}
		for (i = 0; i < comm_size; i++)
		{
			scatter_disp[i] = scatter_sum;
			scatter_sum += scatter_count_arr[i];
		}

		for (i = 0; i < comm_size; i++)
			printf("Rank#%d has been assigned %d spheres.\n", i, scatter_count_arr[i]);

		spheres = malloc(sizeof(sphere) * sphere_count);	
		for (i = 0; i < sphere_count; i++)
		{
			spheres[i].radius = get_rand(1, 5);
			spheres[i].x = get_rand(-20, 20);
			spheres[i].y = get_rand(-20, 20);
			spheres[i].z = get_rand(-20, 20);
		}	
	}

	// Broadcast number of samples to run
	MPI_Bcast(&sample_count, 1, MPI_INT, 0, MPI_COMM_WORLD);

	// Scatter the number of spheres per process
	MPI_Scatter(scatter_count_arr, 1, MPI_INT, &scatter_count, 1, MPI_INT, 0, MPI_COMM_WORLD); 

	// Allocate memory for storing local sphere objects
	recv_spheres = malloc(sizeof(sphere) * scatter_count);
	memset(recv_spheres, 0, sizeof(sphere) * scatter_count);

	// Divide spheres amoung processes
	MPI_Scatterv(spheres, scatter_count_arr, scatter_disp, my_mpi_type, recv_spheres, scatter_count, my_mpi_type, 0, MPI_COMM_WORLD); 

	MPI_Barrier(MPI_COMM_WORLD);
	for (i = 0; i < scatter_count; i++)
	{
		if (recv_spheres[i].radius)
			printf("Rank#%d(%d) has sphere r%f x%f y%f z%f\n", comm_rank, scatter_count, recv_spheres[i].radius, recv_spheres[i].x, recv_spheres[i].y, recv_spheres[i].z);
		bb.x_min = MIN(bb.x_min, (recv_spheres[i].x - recv_spheres[i].radius));
		bb.x_max = MAX(bb.x_max, (recv_spheres[i].x + recv_spheres[i].radius));
		bb.y_min = MIN(bb.y_min, (recv_spheres[i].y - recv_spheres[i].radius));
		bb.y_max = MAX(bb.y_max, (recv_spheres[i].y + recv_spheres[i].radius));
		bb.z_min = MIN(bb.z_min, (recv_spheres[i].z - recv_spheres[i].radius));
		bb.z_max = MAX(bb.z_max, (recv_spheres[i].z + recv_spheres[i].radius)); 
	}

	MPI_Barrier(MPI_COMM_WORLD);
	printf("Rank#%d bounding box: x(%f,%f) y(%f,%f) z(%f,%f)\n", comm_rank, bb.x_min, bb.x_max, bb.y_min, bb.y_max, bb.z_min, bb.z_max);

	MPI_Finalize();
	if (0 == comm_rank)
	{
		free(scatter_count_arr);
		free(scatter_disp);
		free(spheres);
	}
	free(recv_spheres);

	return 0;
}
