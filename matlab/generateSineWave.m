NUM_OF_POINTS = 5000;
Fb = 1e6;                   %base frequency 1MHz
A = 5;                      %amplitude
t = 0:1:(NUM_OF_POINTS-1);  %time vector

% Generate random frequency row vector and then order
Fr = randi(1e3, 1, NUM_OF_POINTS);
Fr = sort(Fr);

F = Fb + Fr;
T = 2*pi*t;
X = F.*T;

y = A * sin(X);
csvwrite('timeData.csv', t);
csvwrite('sinData.csv', y);
plot(y, '-');