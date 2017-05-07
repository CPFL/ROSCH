/*
 * resbench.c		Copyright (C) Shinpei Kato	
 *
 * Resource limit schedulability benchmarking program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>
#include "resbench.h"

void help(void)
{
	printf("Options:\n");
	printf("--cpus=		the number of processors used.\n");
	printf("--umin=		the minimum utilization [0, 1.0] of every individual task.\n");
	printf("--umax=		the maximum utilization [0, 1.0] of every individual task.\n");
	printf("--pmin=		the minimum period of every individual task (by milliseconds).\n");
	printf("--pmax=		the maximum period of every individual task (by milliseconds).\n");
	printf("--dist=		the distribution of utilizations of tasks (by uniform, bimodal, or exponential). if bimodal is chosen, the utilization separator between light tasks and heavy tasks must be set by --sep.\n");
	printf("--sep=		the utilization separator [0, 1.0] between light tasks and heavy tasks in a bimodal distribution.\n");
	printf("--prob=		the probability [0, 1.0] of being heavy tasks in a bimodal distribution.\n");
	printf("--mean=		the mean value [0, 1.0] in an exponetial distribution.\n");
	printf("--deadline=	the type of relative deadlines (by implicit, constrained, or arbitrary). (i) if \"implicit\" is chosen, the relative deadline is equal to the period. (ii) if \"constrained\" is chosen, the relative deadline is set uniformly between the exec. time and the period. (iii) if \"arbitrary\" is chosen, the relative deadline is set uniformly between the exec. time and 4 times the period.\n");
	printf("--period=	the type of periods (by arbitrary or harmonic).\n");
	printf("--time=		the length of benchmarking time (by millisecond).\n");
	printf("--start=	benchmarking starts from this system load [0, 100].\n");
	printf("--end=		benchmarking ends at this system load [0, 100].\n");
	printf("--step=		the distance of every successive sampling system load to be tested by benchmarking.\n");
	printf("--quantity=	the number of tasksets tested per workload.\n");
	printf("--result=	the file name, in which benchmarking results are saved.\n");
	printf("--print		print the progress of benchmarking.\n");
}

/* entry point. */
int main(int argc, char *argv[])
{
	int i, j, k;
	/* temporary vaiables. */
	int tmp;
	char strtmp[MAX_BUF];
	/* the number of processors. */
	int m = NR_RT_CPUS; 
	/* path to a directory where a taskset file is located. */
	char path[MAX_BUF];
	/* path to a taskset file. */
	char tsfile[MAX_BUF];
	FILE *fp;
	/* minimum/maximum utilization of individual task [0, 1.0]. 
	   default: umin=0.1, umax=1.0. */
	char umin[MAX_BUF] = "0.1", umax[MAX_BUF] = "1.0";
	/* minimum/maximum period of individual task (milliseconds).
	   default: pmin=10, pmax=1000. */
	char pmin[MAX_BUF] = "10", pmax[MAX_BUF] = "1000";
	/* distribution of utilization of tasks. 
	   default: uniform distribution. */
	char dist[MAX_BUF] = "uniform";
	/* parameters for bimodal & exponential distributions. 
	   default: all 0.5. */
	char sep[MAX_BUF] = "0.5";
	char prob[MAX_BUF] = "0.5";
	char mean[MAX_BUF] = "0.5";
	/* type of deadlines. default: implicit. */
	char deadline[MAX_BUF] = "implicit";
	/* type of periods. default: nonharmonic. */
	char period[MAX_BUF] = "nonharmonic";
	/* the length of benchmarking (ms). default: 1000 seconds.*/
	int time = 1000000;
	/* benchmarking range [0, 100]
	   default: from system utilization 50% to 100% at every 5%. */
	int start = 50;
	int end = 100;
	int step = 5; 
	/* the number of tasksets per workload testing. */
	int quantity = 1000; 
	/* file name that outputs benchmarking results. */
	char result[MAX_BUF] = "./result";
	/* print flag. */
	int print = 0;
	/* loop counts that consume 1ms. */
	int loop_1ms;
	/* benchmarking workload. */
	int workload;
	/* success ratio. */
	int nr_success;
	double *success_ratios;

	/***********************************************************************
	 * Options:
	 * --cpus= 		the number of processors used.
	 * --umin=		the minimum utilization [0, 1.0] of every individual task.
	 * --umax=		the maximum utilization [0, 1.0] of every individual task.
	 * --pmin=		the minimum period of every individual task 
	 *				(by milliseconds).
	 * --pmax=		the maximum period of every individual task 
	 *				(by milliseconds).
	 * --dist=		the distribution of utilizations of tasks 
	 *				(by uniform, bimodal, or exponential).
	 *				if bimodal is chosen, the utilization separator between
	 *				light tasks and heavy tasks must be set by --sep.
	 * --sep=		the utilization separator [0, 1.0] between light tasks 
	 *				and heavy tasks in a bimodal distribution .
	 * --prob=		the probability [0, 1.0] of being heavy tasks in a 
	 *				bimodal distribution.
	 * --mean=		the mean value [0, 1.0] in an exponetial distribution.
	 * --deadline=	the type of relative deadlines 
	 *				(by implicit, constrained, or arbitrary).
	 *				if "implicit" is chosen, the relative deadline is equal
	 *				to the period.
	 *				if "constrained" is chosen, the relative deadline is set
	 *				uniformly between the exec. time and the period.
	 *				if "arbitrary" is chosen, the relative deadline is set
	 *				uniformly between the exec. time and 4 times the period.
	 * --period=	the type of periods (by arbitrary or harmonic).
	 * --time=		the length of benchmarking time (ms).
	 * --start=		benchmarking starts from this system load [0, 100].
	 * --end=		benchmarking ends at this system load [0, 100].
	 * --step=		the distance of every successive sampling system load
	 *				to be tested by benchmarking.
	 * --quantity=	the number of tasksets tested per workload.
	 * --result=	the file name, in which benchmarking results are saved.
	 * --print		print the progress of benchmarking.
	 ***********************************************************************/
	for (i = 1; i < argc; i++) {
		if (strncmp(argv[i], "--cpus", (tmp = strlen("--cpus"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			m = atoi(&argv[i][tmp+1]);
		}
		else if (strncmp(argv[i], "--umin", (tmp = strlen("--umin"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			strncpy(umin, &argv[i][tmp+1], MAX_BUF);
		}
		else if (strncmp(argv[i], "--umax", (tmp = strlen("--umax"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			strncpy(umax, &argv[i][tmp+1], MAX_BUF);
		}
		else if (strncmp(argv[i], "--pmin", (tmp = strlen("--pmin"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			strncpy(pmin, &argv[i][tmp+1], MAX_BUF);
		}
		else if (strncmp(argv[i], "--pmax", (tmp = strlen("--pmax"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			strncpy(pmax, &argv[i][tmp+1], MAX_BUF);
		}
		else if (strncmp(argv[i], "--dist", (tmp = strlen("--dist"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			strncpy(dist, &argv[i][tmp+1], MAX_BUF);
		}
		else if (strncmp(argv[i], "--sep", (tmp = strlen("--sep"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			strncpy(sep, &argv[i][tmp+1], MAX_BUF);
		}
		else if (strncmp(argv[i], "--prob", (tmp = strlen("--prob"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			strncpy(prob, &argv[i][tmp+1], MAX_BUF);
		}
		else if (strncmp(argv[i], "--mean", (tmp = strlen("--mean"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			strncpy(mean, &argv[i][tmp+1], MAX_BUF);
		}
		else if (strncmp(argv[i], "--deadline", 
						 (tmp = strlen("--deadline"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			strncpy(deadline, &argv[i][tmp+1], MAX_BUF);
		}
		else if (strncmp(argv[i], "--period", 
						 (tmp = strlen("--period"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			strncpy(period, &argv[i][tmp+1], MAX_BUF);
		}
		else if (strncmp(argv[i], "--time", 
						 (tmp = strlen("--time"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			time = atoi(&argv[i][tmp+1]);
		}
		else if (strncmp(argv[i], "--start", (tmp = strlen("--start"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			start = atoi(&argv[i][tmp+1]);
		}
		else if (strncmp(argv[i], "--end", (tmp = strlen("--end"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			end = atoi(&argv[i][tmp+1]);
		}
		else if (strncmp(argv[i], "--step", (tmp = strlen("--step"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			step = atoi(&argv[i][tmp+1]);
		}
		else if (strncmp(argv[i], "--quantity",
						 (tmp = strlen("--quantity"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			quantity = atoi(&argv[i][tmp+1]);
		}
		else if (strncmp(argv[i], "--result", 
						 (tmp = strlen("--result"))) == 0) {
			if (argv[i][tmp] != '=') {
				printf("option \"%s\" is invalid.\n", argv[i]);
				exit(1);
			}
			strncpy(result, &argv[i][tmp+1], MAX_BUF);
		}
		else if (strncmp(argv[i], "--print", (tmp = strlen("--print"))) == 0) {
			print = 1;
		}
		else if (strncmp(argv[i], "--help", (tmp = strlen("--help"))) == 0) {
			help();
			exit(0);
		}
		else {
			printf("option \"%s\" is invalid.\n");
			exit(1);
		}
	}

	/* arrays to store the success ratios. */
	success_ratios = (double*) malloc(sizeof(double) * ((end-start)/step+1));

	/* benchmarking range of system utilization. */
	start *= m;
	end *= m;
	step *= m;
	i = 0;
	for (workload = start; workload <= end; workload += step) {
		/* generate task set files, if necessary. */
		generate_taskset(path, quantity, dist, deadline, period, workload,
						 atof(sep), atof(prob), atof(mean),
						 atof(umin), atof(umax), 
						 atoi(pmin), atoi(pmax));
		/* success counter. */
		nr_success = 0;

		/* schedule 1,000 tasksets, each of which include tasks with
		   total workload = @workload. 
		   dont use @i! */
		for (k = 1; k <= quantity; k++) {
			/* path to the taskset file. */
			sprintf(tsfile, "%s/workload%d/ts%d", 
					path, workload, k);

			if ((fp = fopen(tsfile, "r")) == NULL) {
				printf("Cannot open file %s\n", tsfile);
				goto end;
			}

			/* schedule this taskset. */
			if (print) {
				printf("scheduling %s...\n", tsfile);
			}
			if (schedule(fp, m, loop_1ms, time)) {
				nr_success++;
			}

			fclose(fp);
		}
		/* success ratio. */
		success_ratios[i] = (double)nr_success / (double)quantity;
		if (print) {
			printf("%d %f\n", workload, success_ratios[i] * 100);
		}
		i++;
	}

	fp = fopen(result, "w");
	i = 0;
	for (workload = start; workload <= end; workload += step) {
		fprintf(fp, "%d %f\n", workload, success_ratios[i] * 100);
		i++;
	}
	fclose(fp);

 end:
	free(success_ratios);

	return 0;
}
