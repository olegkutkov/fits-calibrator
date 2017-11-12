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

#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#include <stdio.h>

#include "calibrator.h"

void calibrate_files(calibrator_params_t *params)
{
	int file_count = 0;
	DIR *dp;
	struct dirent *ep;
	size_t inpath_len;
	size_t fname_len;
	char *full_path;

	dp = opendir(params->inpath);

	if (dp == NULL) {
		return;
	}

	while ((ep = readdir(dp))) {
		if (strstr(ep->d_name, "fit") || strstr(ep->d_name, "FIT")) {

			inpath_len = strlen(params->inpath);
			fname_len = strlen(ep->d_name);
			full_path = (char *) malloc(inpath_len + fname_len + 2);

			strncpy(full_path, params->inpath, inpath_len);
			full_path[inpath_len] = '/';
			strncpy(full_path + inpath_len + 1, ep->d_name, fname_len);
			full_path[inpath_len + fname_len + 1] = '\0';

			printf("%s\n", full_path);

			file_count++;
		}
	}
}

