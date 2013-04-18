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