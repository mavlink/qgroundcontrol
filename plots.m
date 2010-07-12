% Attitude tuning

clc;
close all;
% load 'controller-log3.txt';
figure; hold on; plot((data(:,1)-data(1,1))/1000,data(:,2),'ob'); plot((data(:,1)-data(1,1))/1000,data(:,5),'og'); plot((data(:,1)-data(1,1))/1000,data(:,12),'or')
hold on;
plot((data(:,1)-data(1,1))/1000,data(:,8),'ok')
% plot(data(:,2),'-b'); plot(data(:,5),'-g'); plot(data(:,9),'-r')
legend('roll set point','roll rm set point','roll')

figure; hold on; plot(data(:,3),'ob'); plot(data(:,6),'og'); plot(data(:,11),'or')
legend('pitch set point','pitch rm set point','pitch')

figure; hold on; plot(data(:,4),'ob'); plot(data(:,7),'og'); plot(data(:,13),'or')
legend('yaw set point','yaw rm set point','yaw')