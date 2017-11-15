/* 
   calibrator.c
    - fits calibrator implementaion

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

#include <limits.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "list.h"
#include "thread_pool.h"
#include "calibrator.h"
#include "fits_handler.h"
#include "file_utils.h"

#undef max
#undef min
#define min(a, b) ((a) < (b)) ? (a) : (b)
#define max(a, b) ((a) > (b)) ? (a) : (b)

static list_node_t *file_list = NULL;
static int total_files_counter = 0;
static char *USER_TIMEZONE = NULL;

typedef struct thread_arg {
	calibrator_params_t *cal_param;
	list_node_t *filelist;
	int file_list_offset;
	int file_list_len;
} thread_arg_t;

int find_best_calibration_files(calibrator_params_t *params, time_t imtime, double exptime, const char *objname, fits_handle_t **cfiles)
{
	return 0;
}

int substract_darks(calibrator_params_t *params, const char *src_file, time_t imtime, double exptime)
{
	char err_buf[32] = { 0 };
	int status = 0, dark_counter = 0;
	DIR *dp;
	struct dirent *ep;
	fits_handle_t *dark_image = NULL;
	char *full_file_path = NULL;
	double dark_exposure, exp_diff, min_exp, max_exp;
	time_t dark_date, timediff_sec, min_time, max_time;

	dp = opendir(params->darkpath);

	if (dp == NULL) {
		return -1;
	}

	while ((ep = readdir(dp))) {
		if (dark_counter >= params->max_calfiles) {
			break;
		}

		if (strstr(ep->d_name, "fit") || strstr(ep->d_name, "FIT")) {
			build_full_file_path(params->darkpath, ep->d_name, &full_file_path);

			status = 0;

			dark_image = fits_handler_new(full_file_path, &status);

			if (status !=0 ) {
				fits_get_status_code_msg(status, err_buf);
				params->logger_msg("\nUnable to process %s error: %s\n", full_file_path, err_buf);
				free(full_file_path);

				continue;
			}

			dark_date = fits_get_observation_dt(dark_image);
			dark_exposure = fits_get_object_exptime(dark_image);

			min_time = min(dark_date, imtime);
			max_time = max(dark_date, imtime);

			timediff_sec = difftime(max_time, min_time);

			if (timediff_sec <= params->max_timediff) {
				min_exp = min(exptime, dark_exposure);
				max_exp = max(exptime, dark_exposure);

				exp_diff = (min_exp / max_exp) * 100;

				if (exp_diff >= params->min_exp_eq_percent) {
					params->logger_msg("\t!!! Found corresponding dark %s to file %s, timediff: %li sec, expdiff %.2f %%\n",
										full_file_path, src_file, timediff_sec, exp_diff);

					dark_counter++;
				}
			}

			fits_handler_free(dark_image);

			free(full_file_path);
		}
	}

	closedir (dp);

	if (dark_counter < params->min_calfiles) {
		return -1;
	}

	return 0;
}

void calibrate_one_file(const char *file, void *arg)
{
	char err_buf[32] = { 0 };
	char object[76];
	int status = 0;
	time_t image_time;
	double image_exptime;
	fits_handle_t *fits_image;
	calibrator_params_t *params = (calibrator_params_t *) arg;

	params->logger_msg("\nWorking %s\n", file);

	fits_image = fits_handler_new(file, &status);

	if (status != 0) {
		fits_get_status_code_msg(status, err_buf);
		params->logger_msg("\nUnable to process %s error: %s\n", file, err_buf);
		return;
	}

	image_time = fits_get_observation_dt(fits_image);
	fits_get_object_name(fits_image, object);
	image_exptime = fits_get_object_exptime(fits_image);

	params->logger_msg("Object: %s\n", object);
	params->logger_msg("Exposure: %f\n", image_exptime);
	params->logger_msg("Image time: %i\n", image_time);

	if (strlen(params->darkpath) > 0) {
		substract_darks(params, file, image_time, image_exptime);
		//calib_image = find_best_calibration_file(params, image_time, object);
	}

	fits_handler_free(fits_image);
}

void *thread_func(void *arg)
{
	thread_arg_t th_arg_local;
	thread_arg_t *th_arg = (thread_arg_t *) arg;
	calibrator_params_t *params = th_arg->cal_param;

	memcpy(&th_arg_local, th_arg, sizeof(thread_arg_t));

	free(th_arg);

	params = th_arg_local.cal_param;

	iterate_list_cb(th_arg_local.filelist, &calibrate_one_file, params,
					th_arg_local.file_list_offset, th_arg_local.file_list_len, &params->run_flag);

	return NULL;
}

void calibrate_files(calibrator_params_t *params)
{
	int file_count = 0;
	DIR *dp;
	struct dirent *ep;
	char *full_path = NULL;
	long int cpucnt;
	int files_per_cpu_int, left_files, i;
	int file_list_offset_next = 0;
	thread_arg_t *thread_params;

	params->logger_msg("Reading directory %s\n", params->inpath);

	dp = opendir(params->inpath);

	if (dp == NULL) {
		return;
	}

	while ((ep = readdir(dp))) {
		if (strstr(ep->d_name, "fit") || strstr(ep->d_name, "FIT")) {
			build_full_file_path(params->inpath, ep->d_name, &full_path);

			file_list = add_object_to_list(file_list, full_path);

			free (full_path);

			file_count++;
		}
	}

	closedir (dp);

	if (file_count == 0) {
		params->logger_msg("Can't find fits files, sorry\n");
		free_list(file_list);
		file_list = NULL;
		return;
	}

	cpucnt = sysconf(_SC_NPROCESSORS_ONLN);

	params->logger_msg("\nStarting calibrator on %li processor cores with %i tasks by core...\n", cpucnt, params->jobs_count);

	cpucnt *= params->jobs_count;

	USER_TIMEZONE = getenv("TZ");

	if (USER_TIMEZONE) {
		USER_TIMEZONE = strdup(USER_TIMEZONE);
	}

	setenv("TZ", "", 1);
	tzset();


	if (cpucnt > file_count) {
		files_per_cpu_int = 1;
		left_files = 0;
	} else {
		files_per_cpu_int = file_count / cpucnt;
		left_files = file_count % cpucnt;
	}

	params->logger_msg("Total files to calibrate: %i\n", file_count);
	params->logger_msg("Files per CPU core: %i, left: %i\n", files_per_cpu_int, left_files);

	total_files_counter = file_count;

	init_thread_pool(cpucnt);

	for (i = 0; i < cpucnt; i++) {
		thread_params = (thread_arg_t*) malloc(sizeof(thread_arg_t));

		thread_params->cal_param = params;
		thread_params->filelist = file_list;

		thread_params->file_list_offset = file_list_offset_next;
		thread_params->file_list_len = files_per_cpu_int;

		if (i == cpucnt - 1) {
			thread_params->file_list_len += left_files;
		}
 
		thread_pool_add_task(thread_func, thread_params);

		file_list_offset_next += files_per_cpu_int;
	}
}

void calibrator_stop(calibrator_params_t *params)
{
	params->run_flag = 0;

	cleanup_thread_pool();

	if (file_list) {
		free_list(file_list);
		file_list = NULL;
	}

	if (USER_TIMEZONE) {
		setenv("TZ", USER_TIMEZONE, 1);
		free(USER_TIMEZONE);
		USER_TIMEZONE = NULL;
	} else {
		unsetenv("TZ");
	}

	tzset();
}

