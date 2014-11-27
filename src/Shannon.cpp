#include "Shannon.h"
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
Shannon::Shannon(chan& channel) {
	C = &channel;
}

//-------------- declaring the theoric algoritmhs implementation
double Shannon::vulnerability(prob& pi) {
	throw 1; //It is not supported
}

double Shannon::cond_vulnerability(prob& pi) {
	throw 1; //It is not supported
}

double Shannon::entropy(prob& pi) {
	if(C->n_rows != pi.size()) {
		throw 1; // X must be equal for both
	}
	double sum_x = 0;
	for(uint x = 0; x < C->n_rows; x++) {
		sum_x += pi.at(x) * (log(pi.at(x)) / log(2));
	}
	return -sum_x;
	// - sum x pi(x) log2 p(x)
	//log2 p(x) = log p(x) / log (2)
}

double Shannon::cond_entropy(prob& pi) {
	if(C->n_rows != pi.size()) {
		throw 1; // X must be equal for both
	}
	double sum_y;
	double sum_x = 0;
	for(uint x = 0; x < C->n_rows; x++) {
		sum_y = 0;
		for(uint y = 0; y < C->n_cols; y++) {
			sum_y += C->at(x, y) * (log(C->at(x, y)) / log(2));
		}
		sum_x += pi.at(x) * sum_y;
	}
	return -sum_x;
	//- sum x p(x) * (sum y C[x,y] log2 C[x,y])
	//log2 C[x,y] = log C[x,y] / log (2)
}

double Shannon::leakage(prob& pi) {
	if(C->n_rows != pi.size()) {
		throw 1; // X must be equal for both
	}
	return(entropy(pi) - cond_entropy(pi));
}

double Shannon::capacity() {
	//implements the Blahut-Arimoto Algorithm
	return 0;
}
