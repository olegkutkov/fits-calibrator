/* 
   fits_handler.c
    - fits io and processing routines

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

#include <string.h>
#include <errno.h>
#include "fits_handler.h"

fits_handle_t *fits_handler_mem_new(int *status)
{
	fits_handle_t *hdl = (fits_handle_t *) malloc(sizeof(fits_handle_t));

	if (!hdl) {
		*status = -errno;
		return NULL;
	}

	memset(hdl, 0, sizeof(fits_handle_t));

	return hdl;
}

fits_handle_t *fits_handler_new(const char *filepath, int *status)
{
	fits_handle_t *hdl = fits_handler_mem_new(status);

	if (!hdl) {
		return NULL;
	}

	fits_open_file(&hdl->src_fptr, filepath, READONLY, status);

	return hdl;
}

int fits_create_image_mem(fits_handle_t *handle, int width, int height)
{
	handle->image = (double*) malloc(width * height);

	if (!handle->image) {
		return -errno;
	}

	handle->width = width;
	handle->height = height;

	return 0;
}

int fits_copy_image(fits_handle_t *handle, fits_handle_t *src)
{
	if (!handle || !src || !src->image) {
		return -EFAULT;
	}

	if (!handle->image) {
		return -ENOMEM;
	}

	memcpy(handle->image, src->image, src->width * src->height);

	return 0;
}

int fits_get_image_size(fits_handle_t *handle)
{
	int status = 0;
	long anaxes[2] = { 1, 1 };

	fits_get_img_size(handle->src_fptr, 2, anaxes, &status);

	handle->width = anaxes[0];
	handle->height = anaxes[1];

	return status;
}

int fits_get_image_w(fits_handle_t *handle)
{
	return handle->width;
}

int fits_get_image_h(fits_handle_t *handle)
{
	return handle->height;
}

int fits_load_image(fits_handle_t *handle)
{
	int status = 0;
	long firstpix[2] = { 1, 1 };
	long npixels = handle->width * handle->height;

	fits_get_image_size(handle);

	fits_read_pix(handle->src_fptr, TDOUBLE, firstpix,
					npixels, NULL, handle->image, NULL, &status);

	return status;
}

void fits_free_image(fits_handle_t *handle)
{
	if (!handle) {
		return;
	}

	if (handle->image) {
		free(handle->image);
	}
}

time_t fits_get_observation_dt(fits_handle_t *handle)
{
	int status = 0;
	char date[FLEN_CARD] = { 0 };
	size_t date_len;
	int year = 0, month = 1, day = 0, hour = 0, minute = 0;
	double second = 0;
	struct tm timeval;

	memset(&timeval, 0, sizeof(struct tm));

	fits_read_key(handle->src_fptr, TSTRING, "DATE-OBS", date, NULL, &status);

	if (status != 0) {
		return 0;
	}

	if (strstr(date, "T") == NULL) {
		date_len = strlen(date);
		date[date_len] = 'T';
		fits_read_key(handle->src_fptr, TSTRING, "TIME-OBS", date + date_len + 1, NULL, &status);

		if (status != 0) {
			date[date_len] = '\0';
			status = 0;
		}
	}

	fits_str2time(date, &year, &month, &day, &hour, &minute, &second, &status);

	timeval.tm_year = year - 1900;
	timeval.tm_mon = month - 1;
	timeval.tm_mday = day;
	timeval.tm_hour = hour;
	timeval.tm_min = minute;
	timeval.tm_sec = (int) second;

	return mktime(&timeval);
}

int fits_get_object_name(fits_handle_t *handle, char *buf)
{
	int status = 0;

	fits_read_key(handle->src_fptr, TSTRING, "OBJECT", buf, NULL, &status);

	return status;
}

double fits_get_object_exptime(fits_handle_t *handle)
{
	int status = 0;
	float result;

	fits_read_key(handle->src_fptr, TFLOAT, "EXPTIME", &result, NULL, &status);

	return result;
}

int fits_substract_dark(fits_handle_t *image, fits_handle_t *dark)
{
	return 0;
}

int fits_substract_bias(fits_handle_t *image, fits_handle_t *bias)
{
	return fits_substract_dark(image, bias);
}

int fits_save_as_new_file(fits_handle_t *handle, const char *filepath)
{
	return 0;
}

void fits_release_file(fits_handle_t *handle)
{
	int status = 0;

	fits_close_file(handle->src_fptr, &status);

	handle->src_fptr = NULL;
}

void fits_handler_free(fits_handle_t *handle)
{
	if (handle) {
		if (handle->src_fptr) {
			fits_release_file(handle);
		}

		free(handle);
		handle = NULL;
	}
}

void fits_get_status_code_msg(int status, char *buf)
{
	if (status < 0) {
		buf = strerror(errno);
	} else {
		fits_get_errstatus(status, buf);
	}
}

