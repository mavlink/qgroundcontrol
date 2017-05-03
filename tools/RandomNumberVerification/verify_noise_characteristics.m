% ****************************************************************************
%  *
%  *   Copyright (C) 2015 PX4 Development Team. All rights reserved.
%  *   Author: Eddy Scott <scott.edward@aurora.aero>
%  *
%  * Redistribution and use in source and binary forms, with or without
%  * modification, are permitted provided that the following conditions
%  * are met:
%  *
%  * 1. Redistributions of source code must retain the above copyright
%  *    notice, this list of conditions and the following disclaimer.
%  * 2. Redistributions in binary form must reproduce the above copyright
%  *    notice, this list of conditions and the following disclaimer in
%  *    the documentation and/or other materials provided with the
%  *    distribution.
%  * 3. Neither the name PX4 nor the names of its contributors may be
%  *    used to endorse or promote products derived from this software
%  *    without specific prior written permission.
%  *
%  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
%  * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
%  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
%  * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
%  * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
%  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
%  * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
%  * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
%  * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
%  * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
%  * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
%  * POSSIBILITY OF SUCH DAMAGE.
%  *
%  ****************************************************************************/
%% Clean work environment
clear all
close all
clc
%% Compile and run the noise cpp program
system('g++ gaussian_noise.cpp -o generate_noise_csv');
mu_des = 0.0;
std_des = 0.02;
var_des = std_des^2;
num_samples = 10000;
num_runs = 100;
if exist('generated_noise.csv','file')
    system('rm generated_noise.csv')
end
system(sprintf('./generate_noise_csv %s %s %s %s',num2str(mu_des),num2str(var_des),num2str(num_runs),num2str(num_samples)))
%% Load the generated noise values
noise_vals = load('generated_noise.csv');
for i = 1:size(noise_vals,1);
    run_mean = mean(noise_vals(i,:));
    run_std = std(noise_vals(i,:));
    mean_array(i) = run_mean;
    std_array(i) = run_std;
end
%%%%% Display statisitcs of means generated
max_mean = max(mean_array);
min_mean = min(mean_array);
max_std = max(std_array);
min_std = min(std_array);

figure
hist(mean_array)
ylabel('Occurances')
xlabel('Mean Value')
title('Histogram of mean values')

figure
hist(std_array)
ylabel('Occurances')
xlabel('Std Value')
title('Histogram of std values')

table(min_mean, mu_des, max_mean, min_std, std_des, max_std)




