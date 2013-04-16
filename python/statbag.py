#!/usr/bin/python
#
# Simson's simple stats. If what you want isn't here, use stats.

import time
import os

class statbag:
    """A simple statistics package for 1 and two dimensional values.
    Also does histograms."""

    def __init__(self):
        self.x = []
        self.y = []
        self.hist = {}                   # single value histogram

    def __add__(self,another):
        new = stats()
        new.x = self.x + another.x
        new.y = self.y + another.y
        return new
    
    def addx(self,x):
        self.x.append(x)
        self.hist[x] = self.hist.get(x,0) + 1

    def addxy(self,x,y):
        self.x.append(x)
        self.y.append(y)

    def count(self):
        return len(self.x)

    def convert_to_float(self):
        for i in xrange(len(self.x)):
            self.x[i] = float(self.x[i])
        for i in xrange(len(self.y)):
            self.y[i] = float(self.y[i])

    def sumx(self):
        sum = 0
        for i in self.x:
            sum += i
        return sum

    def sumy(self):
        sum = 0
        for i in self.y:
            sum += i
        return sum

    def sumxx(self):
        sum = 0
        for i in self.x:
            sum += i*i
        return sum

    def sumyy(self):
        sum = 0
        for i in self.y:
            sum += i*i
        return sum

    def average(self):
        for i in range(len(self.x)):
            if(type(self.x[i])==type("")): self.x[i] = float(self.x[i])
        return float(self.sumx()) / self.count()

    def minx(self):
        min = self.x[0]
        for i in self.x:
            if(i<min): min=i
        return min

    def maxx(self):
        max = self.x[0]
        for i in self.x:
            if(i>max): max=i
        return max

    def rangex(self):
        return self.maxx() - self.minx()

    def variance(self):
        avg = self.average()
        var = 0
        for i in self.x:
            var  += (i - avg) * (i - avg)
        return var
            
    def stddev(self):
        import math
        return math.sqrt(self.variance() / self.count())

    # Two variable statistics
    def sumxy(self):
        assert(len(self.x)==len(self.y))
        sum = 0
        for i in range(len(self.x)):
            sum += self.x[i]*self.y[i]
        return sum

    def correlation(self):
        import math
        n = len(self.x)
        sumx = self.sumx()
        sumy = self.sumy()
        sumxx = self.sumxx()
        sumyy = self.sumyy()
        sumxy = self.sumxy()
        top = n * sumxy - sumx*sumy
        bot = math.sqrt(( n * sumxx - sumx*sumx) * (n * sumyy - sumy*sumy))
        if(bot==0): return 0            # not correlated
        return top / bot

    def xystr(self):
        """ Return a string of all the xy values """
        ret = ""
        for i in range(len(self.x)):
            ret += "%g %g\n" % (self.x[i],self.y[i])
        return ret

    def stats1(self):
        ret = ""
        ret += "Single variable stats:\n"
        ret += "count= %d\n" % self.count()
        ret += "min: %g max: %g range: %g\n" % (self.minx(),self.maxx(),self.rangex())
        ret += "sum: %g  sum of squares:  %g \n" % (self.sumx(), self.sumxx())
        ret += "average: %g\n" % (self.average())
        ret += "variance: %g  stddev: %g\n" % (self.variance(),self.stddev())
        return ret

    def print_stats1(self):
        print("Single variable stats:")
        print("count= %d" % self.count())
        print("min: %g max: %g range: %g" % (self.minx(),self.maxx(),self.rangex()))
        print("sum: %g  sum of squares:  %g " % (self.sumx(), self.sumxx()))
        print("average: %g" % (self.average()))
        print("variance: %g  stddev: %g" % (self.variance(),self.stddev()))

    def histogram(self):
        "Return a histogram --- a hash of (xvalue,count) tuples"
        return self.hist
        
    def print_histogram(self,xtitle,ytitle):
        "Print a histogram given XTITLE and YTITLE"
        print("%20s %10s" % (xtitle,ytitle))
        k = self.hist.keys()
        k.sort()
        for i in k:
            print("%20s %10d" % (i,self.hist[i]))
        
    def plot_date_histogram(self,fname,title,width,height):
        def add_days(date,days):
            return time.localtime(time.mktime(date)+60*60*24*days)[0:3] + (0,0,0,0,0,0)

        first = add_days(self.minx(),-1) # start one day before
        last  = add_days(self.maxx(),1)  # go to one day after

        cmd_file = fname+".txt"
        dat_file = fname+".dat"

        d = open(dat_file,"w")

        # Generate output for every day...
        # And generate a "0" for every day that we don't have an entry
        # that follows an actual day...
        hist = self.histogram()
        k = hist.keys()
        k.sort()
        for i in k:
            # Check for the previous day
            yesterday = add_days(i,-1)
            if(not hist.has_key(yesterday)):
                d.write("%d/%d/%d  0\n" % (yesterday[1],yesterday[2],yesterday[0]))
            d.write("%d/%d/%d   %d\n" % (i[1],i[2],i[0],hist[i]))
            # Check for the next day
            tomorrow = add_days(i,1)
            if(not hist.has_key(tomorrow)):
                d.write("%d/%d/%d  0\n" % (tomorrow[1],tomorrow[2],tomorrow[0]))
        d.close()

        f = open(cmd_file,"w")
        f.write("set terminal png small size %d,%d\n" % (width,height)) # "small" is fontsize
        f.write("set output '%s'\n" % fname)
        f.write("set xdata time\n")
        f.write("set timefmt '%m/%d/%y'\n")
        f.write("set xrange ['%d/%d/%d':'%d/%d/%d']\n" %
                (first[1],first[2],first[0], last[1],last[2],last[0]+1))
        f.write("set format x '%m/%d'\n")
        f.write("set boxwidth 0.5 relative\n")
        f.write("plot '%s' using 1:2 t '%s' with boxes fs solid\n" % (dat_file,title))
        f.write("quit\n")
        f.close()
        os.system("gnuplot %s" % cmd_file)
	#os.unlink(cmd_file)
        #os.unlink(dat_file)



if __name__ == "__main__":
   import sys

   print("Enter your numbers on a line seperated by spaces:")
   j = sys.stdin.readline()
   st = statbag()
   for v in j.strip().split(' '):
      st.addx(float(v))
   st.print_stats1()
