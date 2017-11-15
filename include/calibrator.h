/* 
   calibrator.h

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

#ifndef __CALIBRATOR_H__
#define __CALIBRATOR_H__

typedef void (*logger_msg_cb) (char*, ...);
typedef void (*done_cb) (void);

typedef struct calibrator_params {
	char inpath[256];
	char outpath[256];
	char darkpath[256];
	char biaspath[256];
	char flatpath[256];
	char run_flag;
	int jobs_count;
	int min_calfiles;
	int max_calfiles;
	long int max_timediff;
	double min_exp_eq_percent;

	logger_msg_cb logger_msg;
	done_cb complete;
} calibrator_params_t;

void calibrate_files(calibrator_params_t *params);
void calibrator_stop();

#endif

