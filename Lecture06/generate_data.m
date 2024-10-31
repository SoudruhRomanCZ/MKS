t = ntc.t;
r=ntc.r;
ad = (r ./ (r+10) ) * 2^10;
ad2 = 0:1023;
p = polyfit(ad, t, 10);
t2 = round(polyval(p, ad2), 1);


figure();
plot(ad,t, "ob");
hold on, plot(ad2, t2, 'r');
dlmwrite('data.dlm', t2*10, ',');