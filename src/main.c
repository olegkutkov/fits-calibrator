/* 
   main.c
    - entry point of the application

   Copyright 2017  Oleg Kutkov <elenbert@gmail.com>

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
#include "version.h"
#include "file_utils.h"
#include "calibrator.h"

static struct option cmd_long_options[] =
{
	{"help",   no_argument, 0, 'h'},
	{"input",   required_argument, 0, 'i'},
	{"output",  required_argument, 0, 'o'},
	{"dark",  required_argument, 0, 'd'},
	{"bias", required_argument, 0, 'b'},
	{"flat", required_argument, 0, 'f'},
	{0, 0, 0, 0}
};

void show_help()
{
	printf("fits-calibrator, version: %i.%i.%i\n"
			, AUTODARK_VERSION_MAJOR, AUTODARK_VERSION_MINOR, AUTODARK_VERSION_PATCH);
	printf("Oleg Kutkov <elenbert@gmail.com>\nCrimean astrophysical observatory, 2017\n\n");

	printf("\t-h, --help\t\tShow this help and exit\n");
	printf("\t-i, --input\t\tSet directory with non-calibrated FITS files\n");
	printf("\t-o, --output\t\tSet directory for resulting calibrated FITS files\n");
	printf("\t-d, --dark\t\tSet directory with darks files\n");
}

int main(int argc, char **argv)
{
	int c;
	calibrator_params_t cparams;
	char *indir = NULL, *outdir = NULL,
		 *darkdir = NULL, *biasdir = NULL, *flatdir = NULL;;

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "hi:o:d:b:f:", cmd_long_options, &option_index);

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

	calibrate_files(&cparams);

    return 0;
}

