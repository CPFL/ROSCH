/*
 * taskset.c	Copyright (C) Shinpei Kato
 *
 * Generate periodic task set files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_BUF 256

typedef struct taskdata_struct {
	int C;		/* computation time */
	int T; 		/* period */
	int D;		/* relative deadline */
	double U;	/* utilization */
} taskdata_t;

static inline int irand(int min, int max)
{
	return min + (int)(rand() * (max - min + 1.0) / (1.0 + RAND_MAX));
}

static inline double drand(double min, double max)
{
	return min + rand() * (max - min) / (double)RAND_MAX;
}

static inline double uniform_dist(double min, double max)
{
	return drand(min, max);
} 

static inline double bimodal_dist(double min, double max, 
								  double sep, double heavy)
{
	int r = rand();
	if ((double)r < (double)RAND_MAX * heavy) {
		/* heavy tasks. */
		return sep + r * (max - sep) / (double)RAND_MAX;
	}
	else {
		/* light tasks. */
		return min + r * (sep - min) / (double)RAND_MAX;
	}
}

static inline double exponential_dist(double mean)
{
	return -mean * (double) log(1.0 - drand(0.0, 1.0));
}

int *harmonic_periods = NULL;
int nr_periods = 0;
static inline void make_harmonic_periods(int pmin, int pmax)
{
	int i;
	int harmonic_period;

	harmonic_period = pmin;
	while (harmonic_period <= pmax) {
		harmonic_period *= 2;
		nr_periods++;
	}

	harmonic_periods = (int*)malloc(sizeof(int) * nr_periods);

	harmonic_period = pmin;
	for (i = 0; i < nr_periods; i++) {
		harmonic_periods[i] = harmonic_period;
		harmonic_period *= 2;
	}
}

void generate_taskset(char *path, int nr_tasksets,
					  char *dist, char *deadline, char *period, int workload, 
					  double sep, double prob, double mean,
					  double umin, double umax, int pmin, int pmax)
{
	int i, k;
	char dir[MAX_BUF];
	char file[MAX_BUF];
	char cmd[MAX_BUF];
	char str[MAX_BUF];
	FILE *fp;
	int nr_tasks;
	double u, utotal, usys = (double)workload / 100.0;
	taskdata_t *taskdata;

	/* create taskset files, only if they do not exist. */
	system("mkdir ./taskset >& .error.log");
	/* create taskset/@dist. */
	sprintf(path, "./taskset/%s", dist);
	sprintf(cmd, "mkdir %s >& .error.log", path);;
	system(cmd);
	/* create taskset/@dist/@deadline. */
	sprintf(path, "%s/%s", path, deadline);
	sprintf(cmd, "mkdir %s >& .error.log", path);;
	system(cmd);
	/* create taskset/@dist/@deadline/@period. */
	sprintf(path, "%s/%s", path, period);
	sprintf(cmd, "mkdir %s >& .error.log", path);
	system(cmd);
	/* create taskset/@dist/@deadline/@parameters. */
	if (strcmp(dist, "uniform") == 0) {
		sprintf(path, "%s/pmin%d_pmax%d_umin%.2f_umax%.2f", 
				path, pmin, pmax, umin, umax);
		sprintf(cmd, "mkdir %s >& .error.log", path);
	}
	else if (strcmp(dist, "bimodal") == 0) {
		sprintf(path, "%s/pmin%d_pmax%d_umin%.2f_umax%.2f_sep%.2f_prob%.2f", 
				path, pmin, pmax, umin, umax, sep, prob);
		sprintf(cmd, "mkdir %s >& .error.log", path);
	}
	else if (strcmp(deadline, "arbitrary") == 0) {
		sprintf(path, "%s/pmin%d_pmax%d_mean%.2f", path, 
				pmin, pmax, mean);
		sprintf(cmd, "mkdir %s >& .error.log", path);
	}
	else {
		printf("Error: undefined deadline type!\n");
		exit(1);
	}
	system(cmd);
	/* create taskset/@dist/@deadline/@period/workload?. */
	sprintf(dir, "%s/workload%d", path, workload);
	sprintf(cmd, "mkdir %s >& .error.log", dir);
	if (system(cmd) == 0) {
		/* initialize random function seed. */
		srand((unsigned int)time(NULL) + workload);

		/* if --period=harmonic is chosen, make harmonic periods. */
		if (strcmp(period, "harmonic") == 0) {
			make_harmonic_periods(pmin, pmax);
		}

		/* generate tasksets for each workload. */
		for (k = 1; k <= nr_tasksets; k++) {
			/* determine utilizations based on a uniform distribution. 
			   results will be written to a tempral file ".util". */
			fp = fopen(".util", "w");
			nr_tasks = 0;
			utotal = 0.0;
			while (1) {
				nr_tasks++;
				if (strcmp(dist, "uniform") == 0) {
					u = uniform_dist(umin, umax);
				}
				else if (strcmp(dist, "bimodal") == 0) {
					u = bimodal_dist(umin, umax, sep, prob);
				}
				else if (strcmp(dist, "exponential") == 0) {
					u = exponential_dist(mean);
				}
				else {
					printf("Error: undefined distribution!\n");
					exit(1);
				}
				utotal += u;
				if (utotal <= usys) {
					fprintf(fp, "%f\n", u);
				}
				else {
					u -= (utotal - usys);
					fprintf(fp, "%f\n", u);
					break;
				}
			}
			fclose(fp);

			/* task data will be stored here. */
			taskdata = (taskdata_t*) malloc(sizeof(taskdata_t) * nr_tasks);

			/* reopen the temporal file to determine C, T, and D. */
			fp = fopen(".util", "r");

			/* determine computation times and periods. */
			for (i = 0; i < nr_tasks; i++) {
				fgets(str, MAX_BUF, fp);
				taskdata[i].U = atof(str);
				if (strcmp(period, "harmonic") == 0) {
					taskdata[i].T = harmonic_periods[rand()%nr_periods];
				}
				else {
					taskdata[i].T = pmin + rand()%(pmax-pmin);
				}
				taskdata[i].C = taskdata[i].U * taskdata[i].T;
			}

			/* determine relative deadlines. */
			if (strcmp(deadline, "implicit") == 0) {
				for (i = 0; i < nr_tasks; i++) {
					taskdata[i].D = taskdata[i].T;
				}
			}
			else if (strcmp(deadline, "constrained") == 0) {
				for (i = 0; i < nr_tasks; i++) {
					taskdata[i].D = taskdata[i].C + 
						rand()%(taskdata[i].T - taskdata[i].C);
				}
			}
			else if (strcmp(deadline, "arbitrary") == 0) {
				for (i = 0; i < nr_tasks; i++) {
					taskdata[i].D = taskdata[i].C + 
						rand()%(4*taskdata[i].T - taskdata[i].C);
				}
			}
			else {
				printf("Error: undefined deadline type!\n");
				exit(1);
			}
			fclose(fp);

			/* generate a taskset file. */
			sprintf(file, "%s/ts%d", dir, k);
			if ((fp = fopen(file, "w")) == NULL) {
				printf("Cannot open %s\n", file);
				return;
			}
			fprintf(fp, "# the number of tasks\n");
			fprintf(fp, "%d\n", nr_tasks);
			fprintf(fp, "# tasks (name, C, T, D)\n");
			for (i = 0; i < nr_tasks; i++) {
				fprintf(fp, "task%d, %d, %d, %d\n", 
						i, taskdata[i].C, taskdata[i].T, taskdata[i].D);
			}
			fclose(fp);

			free(taskdata);
		}
		/* if --period=harmonic is chosen, free harmonic periods. */
		if (strcmp(period, "harmonic") == 0) {
			free(harmonic_periods);
		}
	}
}
