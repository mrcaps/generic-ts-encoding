library(plyr)
library(ggplot2)
library(RColorBrewer)
library(grid)
library(RSQLite)

setwd(dirname(sys.frame(1)$ofile))
theme_update(plot.margin = unit(c(0,0,0,0), "cm"))

fname = "G:/tmp/DATA_from_SIGMOD04_Cai_Ng/stocks/table(18).csv/vs/1_open"

fp = file(fname, "rb")
ints = as.numeric(readBin(fp, integer(), size=2, n=5183))
close(fp)

df = data.frame(v = diff(ints[1:200]))

#deltas are not always small log_2 powers...
ggplot(df, aes(x=v)) + geom_histogram()