import matplotlib.pyplot as plt
import numpy as np
#file:///home/benoitj/Documents/computer/arduino/atmwm/experimental/plot.py

#from matplotlib.ticker import EngFormatter, IndexFormatter

#fig, ax = plt.subplots()
#ax.set_xscale('linear')
#formatter = IndexFormatter('step')
#ax.xaxis.set_major_formatter(formatter)

#xs = np.linspace(0.0, 1000, num=100)
#ys = -0.3 * xs * xs * xs  + 4.5 * xs * xs - 290 * xs + 16100
#ys = (-564.0  * xs * xs * xs + 846.0 * xs * xs - 423.0 * xs + 173.0) * 100;

xMax = 2048

y0 = 17300
yMax = 3200
yMid = yMax + (y0 - yMax) / 2
steps = 8

def value(x):
    if x < xMax / 2:
        x = x / xMax * np.pi
        return (yMid-yMax) * (np.sin(x + np.pi) +1) + yMid
    else:
        x = x / xMax * np.pi
        return (yMid-yMax) * (np.sin(x ) +1)  - 3850



# upper part
xArray = np.linspace(0, xMax, num=steps+1)
yList = []
diffList = []

yLast = 0
xStep = xMax / steps
i = 0
for x in xArray:
    
    y = value(x)
    diff = (yLast -y ) / xStep * 256
    y = int(y)
    yList.append(y)
    if i == 0:
        diff = 0 
    print ('%s %s %s' % (int(x), y, int(diff)))
    yLast = y
    i += 1    

yArray = np.array(yList)

plt.plot(xArray, yArray)

plt.show()

