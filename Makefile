#
# Makefile
#
#   Copyright 2022  Oleg Kutkov <contact@olegkutkov.me>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.  
#

CC := gcc 
PROGRAM = fits-calibrator

DEBUG := -g -ggdb

CFLAGS := -Wall -pipe -I./include -I/usr/include/cfitsio -O2 #$(DEBUG)
LDFLAG := -lcfitsio -pthread

SRC := src/main.c src/file_utils.c src/calibrator.c src/list.c \
		src/thread_pool.c src/fits_handler.c

.PHONY: all
all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAG) -o $(PROGRAM)

.PHONY: install
install:
	cp $(PROGRAM) /usr/bin

.PHONY: clean
clean:
	rm -fr $(PROGRAM) $(PROGRAM).o

