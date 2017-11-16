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
#include <libgen.h>
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

void free_darks_list(fits_handle_t **list, int cnt)
{
	int i = 0;

	for (; i < cnt; ++i) {
		fits_handler_free(list[i]);
	}

	free(list);
}

fits_handle_t *build_master_calibration_file(calibrator_params_t *params, char *dpath,
			int *count, const char *src_file,
			time_t imtime, double exptime)
{
	DIR *dp;
	struct dirent *ep;
	char *full_file_path = NULL;
	char err_buf[32] = { 0 };
	int status = 0, dark_counter = 0;
	double dark_exposure, exp_diff, min_exp, max_exp;
	time_t dark_date, timediff_sec, min_time, max_time;
	fits_handle_t *curr_dark = NULL;
	fits_handle_t *master_file = NULL;

	*count = 0;

	dp = opendir(dpath);

	if (dp == NULL) {
		return NULL;
	}

	while ((ep = readdir(dp))) {
		if (dark_counter >= params->max_calfiles) {
			break;
		}

		if (strstr(ep->d_name, "fit") || strstr(ep->d_name, "FIT")) {
			build_full_file_path(dpath, ep->d_name, &full_file_path);

			status = 0;

			curr_dark = fits_handler_new(full_file_path, &status);

			if (status !=0 ) {
				fits_get_status_code_msg(status, err_buf);
				params->logger_msg("\nUnable to process %s error: %s\n", full_file_path, err_buf);
				free(full_file_path);

				continue;
			}

			dark_date = fits_get_observation_dt(curr_dark);
			dark_exposure = fits_get_object_exptime(curr_dark);

			min_time = min(dark_date, imtime);
			max_time = max(dark_date, imtime);

			timediff_sec = difftime(max_time, min_time);

			if (timediff_sec <= params->max_timediff) {
				min_exp = min(exptime, dark_exposure);
				max_exp = max(exptime, dark_exposure);

				if (exptime > 0) {
					exp_diff = (min_exp / max_exp) * 100;
				} else {
					exp_diff = 100;
				}

				if (exp_diff >= params->min_exp_eq_percent) {
					params->logger_msg("\tFound corresponding calibration %s to file %s, timediff: %li sec, expdiff %.2f %%\n",
											full_file_path, src_file, timediff_sec, exp_diff);

					fits_load_image(curr_dark);

					if (dark_counter == 0) {
						status = 0;

						master_file = fits_handler_mem_new(&status);

						fits_create_image_mem(master_file, fits_get_image_w(curr_dark), fits_get_image_h(curr_dark));

						fits_copy_image(master_file, curr_dark);
					} else {
						fits_add_image_matrix(master_file, curr_dark);
					}

					fits_free_image(curr_dark);

					dark_counter++;
				}
			}

			fits_handler_free(curr_dark);

			free(full_file_path);
		}
	}

	closedir (dp);

	if (exptime > 0) {
		if (dark_counter < params->min_calfiles) {
			if (master_file) {
				free(master_file);
			}

			params->logger_msg("!!! To few (%i) calibration files for the %s skipping calibration...\n", dark_counter, src_file);

			return NULL;
		}
	}

	*count = dark_counter;

	return master_file;
}

int substract_darks(calibrator_params_t *params, fits_handle_t *orig_img, const char *src_file, time_t imtime, double exptime)
{
	int dark_counter = 0;
	int bias_counter = 0;
	fits_handle_t *master_bias;
	fits_handle_t *master_dark = build_master_calibration_file(params, params->darkpath, &dark_counter, src_file, imtime, exptime);

	if (!master_dark) {
		return -1;
	}

	master_bias = build_master_calibration_file(params, params->biaspath, &bias_counter, src_file, imtime, 0);

	if (bias_counter > 0) {
		fits_divide_image_matrix(master_bias, bias_counter);

		fits_substract_image_matrix(master_dark, master_bias);
		fits_substract_image_matrix(orig_img, master_bias);
	}

	fits_divide_image_matrix(master_dark, dark_counter);

	fits_substract_image_matrix(orig_img, master_dark);

	fits_free_image(master_dark);
	fits_free_image(master_bias);

	if (master_dark) {
		free(master_dark);
	}

	if (master_bias) {
		free(master_bias);
	}

	return dark_counter;
}

void calibrate_one_file(const char *file, void *arg)
{
	char err_buf[32] = { 0 };
	char object[76] = { 0 };
	char comment[35] = { 0 };
	int status = 0, count = 0;
	time_t image_time;
	double image_exptime;
	fits_handle_t *fits_image;
	char *save_path = NULL;
	char *target_basename = NULL;
	calibrator_params_t *params = (calibrator_params_t *) arg;

	params->logger_msg("\nWorking %s\n", file);

	target_basename = basename((char*)file);
	build_full_file_path(params->outpath, target_basename, &save_path);

	if (is_file_exist(save_path)) {
		params->logger_msg("File %s is already exists, skipping calibration\n", save_path);
		free(save_path);

		total_files_counter--;

		if (total_files_counter == 0) {
			params->complete();
		}

		return;
	}

	fits_image = fits_handler_new(file, &status);

	if (status != 0) {
		fits_get_status_code_msg(status, err_buf);
		params->logger_msg("\nUnable to process %s error: %s\n", file, err_buf);
		return;
	}

	image_time = fits_get_observation_dt(fits_image);
	fits_get_object_name(fits_image, object);
	image_exptime = fits_get_object_exptime(fits_image);

	if (strlen(params->darkpath) > 0) {
		fits_load_image(fits_image);

		if ((count = substract_darks(params, fits_image, file, image_time, image_exptime)) > 0) {

			snprintf(comment, 25, "Calibrated using %i darks", count);

			fits_save_as_new_file(fits_image, save_path, comment);

			free(save_path);

		}

		fits_free_image(fits_image);
	}

	fits_handler_free(fits_image);


	task_enter_critical_section();

	total_files_counter--;

	if (total_files_counter == 0) {
		params->complete();
	}

	task_exit_critical_section();
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

