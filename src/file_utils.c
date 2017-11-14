/* 
   file_utils.c
    - auxilarity file's functions

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
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

long get_file_size(char *fname)
{
	struct stat statbuf;

	if (stat(fname, &statbuf) < 0) {
		return 0;
	}

	return statbuf.st_size;
}

int is_file_exist(char *filename)
{
	struct stat buffer;
	return (stat (filename, &buffer) == 0);
}

int remove_file(const char *filename)
{
	return unlink(filename);
}

int is_regular_file(const char *path)
{
    struct stat path_stat;

    stat(path, &path_stat);

    return S_ISREG(path_stat.st_mode);
}

void build_full_file_path(const char *dir, const char *file, char **dst)
{
	size_t dir_path_len = strlen(dir);
	size_t fname_len = strlen(file);
	*dst = (char *) malloc(dir_path_len + fname_len + 2);

	strncpy(dst, dir, dir_path_len);
	dst[dir_path_len] = '/';
	strncpy(dst + dir_path_len + 1, file, fname_len);
	dst[dir_path_len + fname_len + 1] = '\0';

	return dst;
}

