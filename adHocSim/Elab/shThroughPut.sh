
#Create a plotable file extracting the throughput information form
#the collectedData.dat file

#xtract rough data
awk -f extractThroughput.awk ../collectedData.dat >thDat1

#sort it
sort thDat1 -n +0 +1 >thDat2

#generate the latency file
awk -f genPlotTh.awk thDat2 > thData.dat

#remove temporary files
rm -f thDat1 thDat2

#create the plot file and plot it!
gnuplot throughput.gp && gv Throughput.ps

