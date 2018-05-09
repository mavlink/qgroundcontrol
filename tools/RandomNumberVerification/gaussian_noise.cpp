/****************************************************************************
 *
 *   Copyright (C) 2015 PX4 Development Team. All rights reserved.
 *   Author: Eddy Scott <scott.edward@aurora.aero>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/



#include <cstdlib>
#include <cmath>
#include <ctime>
#include <limits>
#include <iostream> // not needed
#include <fstream> // not needed
using namespace std;
float generateGaussianNoise(float mu, float variance)
{
	/* Calculate normally distributed variable noise with mean = mu and variance = variance.  Calculated according to 
	Box-Muller transform */
	static const float epsilon = std::numeric_limits<float>::min(); //used to ensure non-zero uniform numbers
	static const float two_pi = 2.0*3.14159265358979323846; // 2*pi
	static float z0; //calculated normal distribution random variables with mu = 0, var = 1;
	float u1, u2;		 //random variables generated from c++ rand();
	/*Generate random variables in range (0 1] */
	do
	{
	    u1 = rand() * (1.0 / RAND_MAX);
	    u2 = rand() * (1.0 / RAND_MAX);
	}
	while ( u1 <= epsilon );  //Have a catch to ensure non-zero for log()

	z0 = sqrt(-2.0 * log(u1)) * cos(two_pi * u2); //calculate normally distributed variable with mu = 0, var = 1
	float noise = z0 * sqrt(variance) + mu; //calculate normally distributed variable with mu = mu, std = var^2
	return noise;
}
int main(int argc, char *argv[])
{
  ofstream fid;
  fid.open ("generated_noise.csv");
  float mu = atof(argv[1]); // Define the mean of the noise, for gaussian = 0
  float variance = atof(argv[2]); //Define the variance of the noise
  int num_runs = atoi(argv[3]); //Define number of runs
  int num_samples = atoi(argv[4]);
  srand(time(NULL)); //Seed rand() function so same random variables are not calculated
  cout << "Desired Mean: " << mu << "\n";
  cout << "Desired Variance: " << variance << "\n";
  cout << "Desired number of runs: " << num_runs << "\n";
  cout << "Desired number of samples per run: " << num_samples << "\n";
  for(int j=0;j<num_runs;j++){
  	if(j!=0){
  		fid <<"\n";
  	}
  	for(int i=0;i<num_samples;i++){
    	fid << generateGaussianNoise(mu, variance) << ",";
  	}
}
  fid.close();
  return 0;
}
