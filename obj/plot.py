import matplotlib.pyplot as plt
import csv
import numpy


data_size=[]
time=[]
XNEW=[]
YNEW=[]
rate=[]
time=[]
time_last=[]
data=[]
sum_=0
time_temp=[]
count=1;

with open('plot.txt','r') as csvfile:
    plots = csv.reader(csvfile, delimiter=',')
    for row in plots:
        time_temp.append(int(row[0]))
        data_size.append(int(row[1]))
        #time_last.append(count)
        #count=count+1

for x in time_temp:
	if(x!=0):
		time.append(x)
		time_last.append(count)
		count=count+1


#for i in range(10):
#	data_size.append(5)	


#use this for no packet loss
#for i in range(10):
#	time.append(i+1)

#use this time for packet loss
#time=[5,11,12,14,16,20,24,32,45,65]

	

for x in data_size:
	sum_=sum_+x
	data.append(sum_)


xtemp=0
ytemp=0

for x,y in zip(data,time):
	XNEW.append(x-xtemp)
	YNEW.append(y-ytemp)
	xtemp=x
	ytemp=y

#for y in YNEW:
#	print(y)


#used as Y axis in graph
for x,y in zip(XNEW,YNEW):
	rate.append(x/y)	

average=numpy.mean(rate)


#used as X axis in graph
#for i in range(10):
#	time_last.append(i+1)
	

plt.plot(time_last,rate, label='delay 100 noloss DATA_SIZE:4 WINDOW_SIZE:10')
plt.xlabel('x')
plt.ylabel('y')
plt.title(average)
plt.legend()
#plt.savefig("delay.png")
plt.show()
