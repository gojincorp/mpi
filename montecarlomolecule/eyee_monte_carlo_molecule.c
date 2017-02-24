#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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

/*
 * Structure for sphere attributes 
 */
typedef struct mystruct
{
	double radius;	// radius of sphere
	double x;	// x coordinate of center of sphere
	double y;	// y coordinate of center of sphere
	double z;	// z coordinate of center of sphere
} sphere;

/*
 * Structure for bounding box attributes 
 */
typedef struct
{
	double x_min;	// minimum x coordinate
	double x_max;	// maximum x coordinate
	double y_min;	// minimum y coordinate
	double y_max;	// maximum y coordinate
	double z_min;	// minimum z coordinate
	double z_max;	// maximum z coordinate
} boundingBox;

/*
 * Main function 
 *
 * @param int argc: count of parameters 
 * @param int[] argv: array of parameters 
 * @return int: return 0 on success 
 */
int main(int argc, char ** arcv)
{
	MPI_Status status;
	int sphere_count = 0, sample_count;
	int comm_size, comm_rank;
	int i, j;
	int scatter_sum = 0;
	int * scatter_count_arr, * scatter_disp;
	sphere * spheres, * recv_spheres;
	boundingBox bb = {DBL_MAX,-DBL_MAX,DBL_MAX,-DBL_MAX,DBL_MAX,-DBL_MAX};
	double bbVol;
	double ** samples;

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

	// Calculate local bounding box
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
	if (scatter_count)
		printf("Rank#%d bounding box: x(%f,%f) y(%f,%f) z(%f,%f)\n", comm_rank, bb.x_min, bb.x_max, bb.y_min, bb.y_max, bb.z_min, bb.z_max);

	// Calculate global bounding box 
	MPI_Reduce(comm_rank ? &bb.x_min : MPI_IN_PLACE, &bb.x_min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);	
	MPI_Reduce(comm_rank ? &bb.x_max : MPI_IN_PLACE, &bb.x_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);	
	MPI_Reduce(comm_rank ? &bb.y_min : MPI_IN_PLACE, &bb.y_min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);	
	MPI_Reduce(comm_rank ? &bb.y_max : MPI_IN_PLACE, &bb.y_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);	
	MPI_Reduce(comm_rank ? &bb.z_min : MPI_IN_PLACE, &bb.z_min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);	
	MPI_Reduce(comm_rank ? &bb.z_max : MPI_IN_PLACE, &bb.z_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);	
	MPI_Barrier(MPI_COMM_WORLD);
	if (0 == comm_rank)
		printf("Global bounding box: x(%f,%f) y(%f,%f) z(%f,%f)\n", comm_rank, bb.x_min, bb.x_max, bb.y_min, bb.y_max, bb.z_min, bb.z_max);

	// Calculate bounding box volume
	bbVol = (bb.x_max - bb.x_min) * (bb.y_max - bb.y_min) * (bb.z_max - bb.z_min);

	// Generate random sample
/*
	if (0 == comm_rank)
	{
		srand(time(NULL) * 1362);
		//samples = (double **)malloc(sizeof(double *) * sample_count + (sizeof(double) * sample_count * 3));
		samples = (double **)malloc(sizeof(double *) * sample_count);
		samples[0] = (double *)malloc(sizeof(double) * 3 * sample_count);
		//samples = malloc(sizeof(double *) * sample_count);
		//samples = calloc(sample_count, sizeof(double *));
		for (i = 0; i < sample_count; i++)
		{
			samples[i] = (*samples + 3 * i);
			samples[i][0] = get_rand(bb.x_min, bb.x_max);
			samples[i][1] = get_rand(bb.y_min, bb.y_max);
			samples[i][2] = get_rand(bb.z_min, bb.z_max);
			printf("Sample #%d (%p | %p | %p | %p | %p): %f %f %f\n", i, &samples, samples, &samples[i], samples[i], &samples[i][0], samples[i][0], samples[i][1], samples[i][2]);
		}	
	}
*/
	srand(time(NULL) * 1362);
	samples = (double **)malloc(sizeof(double *) * sample_count);
	samples[0] = (double *)malloc(sizeof(double) * 3 * sample_count);
	for (i = 0; i < sample_count; i++)
	{
		samples[i] = (*samples + 3 * i);
		if (0 == comm_rank)
		{
			samples[i][0] = get_rand(bb.x_min, bb.x_max);
			samples[i][1] = get_rand(bb.y_min, bb.y_max);
			samples[i][2] = get_rand(bb.z_min, bb.z_max);
			//printf("Sample #%d (%p | %p | %p | %p | %p): %f %f %f\n", i, &samples, samples, &samples[i], samples[i], &samples[i][0], samples[i][0], samples[i][1], samples[i][2]);
		}
	}	

	// Broadcast samples
	MPI_Bcast(samples[0], 3 * sample_count, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);

	// Evaluate sample intersection with local spheres
	bool hit[sample_count];
	double distSqrd;
	for (i = 0; i < sample_count; i++)
	{
		hit[i] = false;
		for (j = 0; j < scatter_count; j++)
		{
			distSqrd = distanceSquared(samples[i],(double[]){recv_spheres[j].x, recv_spheres[j].y, recv_spheres[j].z});
			hit[i] = (distSqrd < recv_spheres[j].radius * recv_spheres[j].radius) ? true : false;
/*
			if (distSqrd < recv_spheres[j].radius * recv_spheres[j].radius)
				printf("Rank#%d | Point(%f,%f,%f) | Sphere(%f | %f,%f,%f)\n", comm_rank, samples[i][0], samples[i][1], samples[i][2], recv_spheres[j].radius, recv_spheres[j].x, recv_spheres[j].y, recv_spheres[j].z); 
*/
		}
	}
	int testCount = 0;
	for (i = 0; i < sample_count; i++)
	{
		if (hit[i])
			testCount++;
	}

	// Merge hit data
	MPI_Reduce(comm_rank ? hit : MPI_IN_PLACE, hit, sample_count, MPI_C_BOOL, MPI_LOR, 0, MPI_COMM_WORLD);

	// Estimate volume
	MPI_Barrier(MPI_COMM_WORLD);
	if (0 == comm_rank)
	{
		int hitCount = 0;
		for (i = 0; i < sample_count; i++)
		{
			if (hit[i])
				hitCount++;
		}
		double moleculeVol = bbVol * ((double)hitCount / (double)sample_count);
		printf("Rank#%d | hits = %d/%d | bbVol = %f |  moleculeVol = %f\n", comm_rank, testCount, hitCount, bbVol, moleculeVol);
	}
 
	MPI_Finalize();
	if (0 == comm_rank)
	{
		free(scatter_count_arr);
		free(scatter_disp);
		free(spheres);
		free(samples[0]);
		free(samples);
	}
	free(recv_spheres);

	return 0;
}
