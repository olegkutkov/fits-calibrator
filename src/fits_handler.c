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

fits_handle_t *fits_handler_new(const char *filepath, int *status)
{
	fits_handle_t *hdl = (fits_handle_t *) malloc(sizeof(fits_handle_t));

	if (!hdl) {
		*status = -errno;
		return NULL;
	}

	memset(hdl, 0, sizeof(fits_handle_t));

	fits_open_file(&hdl->src_fptr, filepath, READONLY, status);

	return hdl;
}

time_t fits_get_observation_dt(fits_handle_t *handle)
{
	int status = 0, nkeys;
	char comment[FLEN_COMMENT];
	char card[FLEN_CARD] = { 0 };
	struct tm timeval;

	fits_get_hdrspace(handle->src_fptr, &nkeys, NULL, &status);

	fits_read_key(handle->src_fptr, TSTRING, "DATE-OBS", card, comment, &status);

	if (strstr(card, "T") == NULL) {
		card[strlen(card)] = 'T';
		fits_read_key(handle->src_fptr, TSTRING, "TIME-OBS", card + strlen(card), comment, &status);
	}

	printf("%s\n", card);

	fits_str2time(card, &timeval.tm_year, &timeval.tm_mon, &timeval.tm_mday, 
					&timeval.tm_hour, &timeval.tm_min, (double *)&timeval.tm_sec, &status);

//	timeval.tm_year = year;

//	printf("y: %i  m: %i d: %i  h: %i  min: %i  sec: %f\n", year, month, day, hour, minute, second);

	return mktime(&timeval);
}

int fits_substract_dark(fits_handle_t *image, fits_handle_t *dark)
{
	return 0;
}

int fits_substract_bias(fits_handle_t *image, fits_handle_t *bias)
{
	return fits_substract_dark(image, bias);
}

void fits_handler_free(fits_handle_t *handle)
{
	int status = 0;

	if (handle) {
		if (handle->src_fptr) {
			fits_close_file(handle->src_fptr, &status);
		}

		free(handle);
	}
}

void get_status_code_msg(int status, char *buf)
{
	if (status < 0) {
		buf = strerror(errno);
	} else {
		fits_get_errstatus(status, buf);
	}
}

