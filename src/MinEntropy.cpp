#include "MinEntropy.h"
/*
This file belongs to the LIBQIF library.
A Quantitative Information Flow C++ Toolkit Library.
Copyright (C) 2013  Universidad Nacional de Río Cuarto(National University of Río Cuarto).
Author: Martinelli Fernán - fmartinelli89@gmail.com - Universidad Nacional de Río Cuarto (Argentina)
LIBQIF Version: 1.0
Date: 12th Nov 2013
========================================================================
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

=========================================================================
*/

double MinEntropy::vulnerability(const prob& pi) {
	return max(pi);
}

double MinEntropy::cond_vulnerability(const prob& pi) {
	check_prior(pi);

	double sum_x;
	double max_x;
	double sum_y = 0;

	for(uint y = 0; y < C.n_cols; y++) {
		max_x = 0;
		for(uint x = 0; x < C.n_rows; x++) {
			sum_x = pi.at(x) * C.at(x, y);
			if(sum_x > max_x) {
				max_x = sum_x;
			}
		}
		sum_y += max_x;
	}
	return sum_y;
	//sum y max x pi(x) C[x,y]
}

double MinEntropy::entropy(const prob& pi) {
	return -log2(vulnerability(pi));
}

double MinEntropy::cond_entropy(const prob& pi) {
	return -log2(cond_vulnerability(pi));
}

double MinEntropy::capacity() {
	double sum_x;
	double max_x;
	double sum_y = 0;

	for(uint y = 0; y < C.n_cols; y++) {
		max_x = 0;
		for(uint x = 0; x < C.n_rows; x++) {
			sum_x = C.at(x, y);
			if(sum_x > max_x) {
				max_x = sum_x;
			}
		}
		sum_y += max_x;
	}
	return log(sum_y);
}
