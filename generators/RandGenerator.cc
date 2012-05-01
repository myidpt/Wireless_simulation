//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "RandGenerator.h"

bool RandGenerator::gsCalled = false;

RandGenerator::RandGenerator() {
	generateSeed();
}

double RandGenerator::uniGen() {
	double ran = rand();
	ran /= RAND_MAX; // Uniform distribution in [0,1].
	return ran;
}

double RandGenerator::expGen(double lamda){
	double tmp = (-log(uniGen()))/lamda;
//	printf("expGen: %lf, lamda=%lf.\n", tmp, lamda);
	return tmp; // Exponential distribution.
}

double RandGenerator::rayGen(double row){
	double tmp = row * sqrt(-2 * log(uniGen()));
//	printf("rayGen: %lf, row=%lf.\n", tmp, row);
	return tmp;
}

bool RandGenerator::boolGen(double prob){
	double tmp = uniGen();
	return tmp < prob;
}

void RandGenerator::generateSeed(){
	if(gsCalled == true)
		return;
	gsCalled = true;
	srand(time(NULL));
//	srand(0);
}

RandGenerator::~RandGenerator() {
}
