function searchtimeHist(bins)
load('search');
figure(1);
hist(search,str2double(bins));
dMEAN= mean(search)
dSTD= std(search)
end
