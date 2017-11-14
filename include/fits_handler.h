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

#ifndef __FITS_HANDLER_H__
#define __FITS_HANDLER_H__

#include <fitsio.h>
#include <time.h>

typedef struct fits_handle {
	fitsfile *src_fptr;
} fits_handle_t;

fits_handle_t *fits_handler_new(const char *filepath, int *status);
time_t fits_get_observation_dt(fits_handle_t *handle);
int fits_substract_dark(fits_handle_t *image, fits_handle_t *dark);
int fits_substract_bias(fits_handle_t *image, fits_handle_t *bias);
void fits_handler_free(fits_handle_t *handle);

void get_status_code_msg(int status, char *buf);

#endif

