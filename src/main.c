/* 
   main.c
    - entry point of the application

   Copyright 2022  Oleg Kutkov <contact@olegkutkov.me>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include "version.h"
#include "file_utils.h"
#include "calibrator.h"

static volatile int RUN_FLAG = 0;

static struct option cmd_long_options[] =
{
	{"help",   no_argument, 0, 'h'},
	{"input",   required_argument, 0, 'i'},
	{"output",  required_argument, 0, 'o'},
	{"dark",  required_argument, 0, 'd'},
	{"bias", required_argument, 0, 'b'},
	{"flat", required_argument, 0, 'f'},
	{"time-diff", required_argument, 0, 't'},
	{"exp-diff", required_argument, 0, 'e'},
	{"min-calfiles", required_argument, 0, 'n'},
	{"max-calfiles", required_argument, 0, 'm'},
	{"jobs", required_argument, 0, 'j'},
	{0, 0, 0, 0}
};

void show_help()
{
	printf("fits-calibrator, version: %i.%i.%i\n"
			, AUTODARK_VERSION_MAJOR, AUTODARK_VERSION_MINOR, AUTODARK_VERSION_PATCH);
	printf("Oleg Kutkov <contact@olegkutkov.me>\n\n");

	printf("\t-h, --help\t\tShow this help and exit\n");
	printf("\t-i, --input\t\tSet directory with non-calibrated FITS files\n");
	printf("\t-o, --output\t\tSet directory for resulting calibrated FITS files\n");
	printf("\t-d, --dark\t\tSet directory with darks files\n");
	printf("\t-b, --bias\t\tSet directory with bias files\n");
	printf("\t-t, --time-diff\t\tSet max time diff between image and calibration file is seconds (default is 86400)\n");
	printf("\t-e, --exp-diff\t\tSet min exposure equality between image and calibration file in percenst (default is 65)\n");
	printf("\t-n, --min-calfiles\tSet minumum requred num of calibration files to process image (default is 2)\n");
	printf("\t-m, --max-calfiles\tSet maximum requred num of calibration files to process image (default is 17)\n");
	printf("\t-j, --jobs\t\tSet threads count per CPU\n");
}

void logger_msg(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vprintf(fmt, args);

	va_end(args);
}

void interrupt_handler(int val)
{
	RUN_FLAG = 0;
}

void calibration_done(void)
{
	RUN_FLAG = 0;
}

int main(int argc, char **argv)
{
	int c;
	calibrator_params_t cparams;
	char *indir = NULL, *outdir = NULL,
		 *darkdir = NULL, *biasdir = NULL, *flatdir = NULL;

	long int timediff_max = 86400;
	double expdiff_min = 65;
	int calfiles_min = 2, calfiles_max = 17, jobs_count = 1;

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "hi:o:d:b:f:t:e:n:m:j:", cmd_long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
			case 'h':
				show_help();
				return 0;

			case 'i':
				indir = optarg;
				break;

			case 'o':
				outdir = optarg;
				break;

			case 'd':
				darkdir = optarg;
				break;

			case 'b':
				biasdir = optarg;
				break;

			case 'f':
				flatdir = optarg;
				break;

			case 't':
				timediff_max = atol(optarg);
				break;

			case 'e':
				expdiff_min = atof(optarg);
				break;

			case 'n':
				calfiles_min = atoi(optarg);
				break;

			case 'm':
				calfiles_max = atoi(optarg);
				break;

			case 'j':
				jobs_count = atoi(optarg);
				break;

			case '?':
				show_help();
				return -1;

			default:
				abort();
		}
	}

	if (indir == NULL || outdir == NULL) {
		fprintf(stderr, "Please set input and output directories\n\n");
		show_help();
		return -1;
	}

	if (darkdir == NULL && biasdir == NULL && flatdir == NULL) {
		fprintf(stderr, "Please set at least one of the directories: dark, bias, flat\n\n");
		show_help();
		return -1;
	}

	if (!is_file_exist(indir)) {
		fprintf(stderr, "Path %s doesn't exists\n", indir);
		return -1;
	}

	if (!is_file_exist(outdir)) {
		fprintf(stderr, "Path %s doesn't exists\n", outdir);
		return -1;
	}

	memset(&cparams, 0, sizeof(calibrator_params_t));

	strcpy(cparams.inpath, indir);
	strcpy(cparams.outpath, outdir);

	if (darkdir != NULL) {
		strcpy(cparams.darkpath, darkdir);
	}

	if (biasdir != NULL) {
		strcpy(cparams.biaspath, biasdir);
	}

	if (flatdir != NULL) {
		strcpy(cparams.flatpath, flatdir);
	}

	RUN_FLAG = 1;
	signal(SIGINT, interrupt_handler);

	cparams.logger_msg = &logger_msg;
	cparams.complete = &calibration_done;

	cparams.min_calfiles = calfiles_min;
	cparams.max_calfiles = calfiles_max;
	cparams.max_timediff = timediff_max;
	cparams.min_exp_eq_percent = expdiff_min;

	cparams.jobs_count = jobs_count;

	cparams.run_flag = 1;

	calibrate_files(&cparams);

	while (RUN_FLAG) {
		sleep(1);
	}

	calibrator_stop(&cparams);

    return 0;
}

