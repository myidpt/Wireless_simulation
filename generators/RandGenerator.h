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

#ifndef RANDGENERATOR_H_
#define RANDGENERATOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <cstdlib>

class RandGenerator{
protected:
	static bool gsCalled;
	static void generateSeed();
public:
	RandGenerator();
	double uniGen();
	double expGen(double);
	double rayGen(double);
	bool boolGen(double);
	virtual ~RandGenerator();
};

#endif /* RANDGENERATOR_H_ */
