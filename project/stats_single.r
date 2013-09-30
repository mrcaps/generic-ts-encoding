library(plyr)
library(ggplot2)
library(RColorBrewer)
library(grid)
library(RSQLite)

setwd(dirname(sys.frame(1)$ofile))
theme_update(plot.margin = unit(c(0,0,0,0), "cm"))

fname = "G:/tmp/noisy_logisticsurrogate_data_set/logisticTrainSignal.txt/vs/31"
fp = file(fname, "rb")
ints = as.numeric(readBin(fp, integer(), size=8, n=1000))
close(fp)

li = length(ints)
nsfirst = c(10,20,40,floor(li/10),li-1)
#df = data.frame(v = diff(ints[1:200]))
df = adply(nsfirst, 1, function(n) {
  data.frame(
    pfx=as.factor(rep(n, n)),
    vals=log2(abs(diff(ints[1:(n+1)])))
  )
})

#deltas are not always small log_2 powers...
ggplot(df) +
  stat_ecdf(aes(x=vals, group=pfx, color=pfx), binwidth=1, size=1) +
  scale_x_continuous("log base 2 of delta") +
  scale_y_continuous("cumulative density") +
  scale_color_hue("Prefix", h=c(20,200))
ggsave("figs/prefix-cdf.pdf",width=5,height=3.5)


#nasa_valve has columns with very low information
# (e.g., an entire column of 2.97363281250000000000)
fname = "G:/tmp/nasa_valve/COL-1-Shunt-COL-2-Temperature-COL-3-Hall-Effect-Sensor/Data Set 1/v37891 S000.TXT/vs/1"
fp = file(fname, "rb")
ints = as.numeric(readBin(fp, integer(), size=8, n=1000))
close(fp)

#inlineskating has data with lots of runs.
fname = "G:/tmp/inline_skating/inline_skating.dat/vs/15_discretizedgluteusmaximus"
fp = file(fname, "rb")
ints = as.numeric(readBin(fp, integer(), size=1, n=1000))
close(fp)

#all 2s and 3s... some sequence redundancy
fname = "G:/tmp/Discord_anomaly_detection/mitdbx_mitdbx_108.txt/vs/1"
fp = file(fname, "rb")
ints = as.numeric(readBin(fp, integer(), size=4, n=100))
close(fp)
ggplot(data.frame(x=seq(0,length(ints)-2),y=diff(ints)), aes(x,y)) + geom_line()

#hmm...
fname = "G:/tmp/Discord_anomaly_detection/qtdbsel102.txt/vs/2"
fp = file(fname, "rb")
ints = as.numeric(readBin(fp, integer(), size=2, n=100))
close(fp)
ggplot(data.frame(x=seq(0,length(ints)-2),y=diff(ints)), aes(x,y)) + geom_line()

#
fname = "G:/tmp/CMU_robot/cement.txt/vs/1"
fp = file(fname, "rb")
ints = as.numeric(readBin(fp, integer(), size=4, n=100000))
close(fp)
ggplot(data.frame(x=diff(ints)), aes(x)) + geom_histogram()
