library(plyr)
library(ggplot2)
library(RColorBrewer)
library(grid)
library(RSQLite)

setwd(dirname(sys.frame(1)$ofile))
theme_update(plot.margin = unit(c(0,0,0,0), "cm"))

outdir = "G:/tmp"

drv = dbDriver("SQLite")
con = dbConnect(drv, dbname=paste(outdir, "combined.db", sep="/"))

dta = dbGetQuery(con, paste("select vpath, vsize, npoints from meta",
  "where vpath in (",
  "'G:/tmp\\noisy_logisticsurrogate_data_set\\logisticTrainSignal.txt/vs/31',",
  "'G:/tmp\\many_small_datasets\\ERP_data.dat/vs/3699',",
  "'G:/tmp\\2DTimeSeries\\ASL_clean\24.dat/vs/2',",
  "'G:/tmp\\DanceGestureData\\Franck_Trajs\\Fra_EnrBass_Moy_Rot.san/vs/36_leftfootz',",
  "'G:/tmp\\dataset_kalpakis\\Population\\population.txt/vs/39_sd',",   
  "'G:/tmp\\shape_mixed_bag\\time_series.txt/vs/208',",
  "'G:/tmp\\long_time_series\\tao.dat/vs/3045',",
  "'G:/tmp\\Mallat_technometrics\\dbtime_train.txt/vs/400')"))
                            

deltafy = function(arr) {
  arr[2:length(arr)] - arr[1:length(arr)-1]
}

stopifnot( deltafy(c(10, 4, 6, 6, 2, 30)) == c(-6, 2, 0, -4, 28) )

toplot = NULL

dbyfile = ddply(dta, .(vpath), function(r) {
  fp = file(r$vpath, "rb")
  print(paste("path was", r$vpath, "size was", r$vsize))
  ints = as.numeric(readBin(fp, integer(), size=r$vsize, n=r$npoints))
  
  #print("deltas")
  #print(length(deltas))
  #print("unique")
  #print(length(unique(deltas)))
  #toplot <<- ints
    
  close(fp)
  data.frame(v = diff(ints))
})

#ggplot(data.frame(v=toplot), aes(x=v)) + geom_histogram()
#ggplot(data.frame(v=deltafy(toplot)), aes(x=v)) + geom_histogram()

#what's up with G:/tmp/DATA_from_SIGMOD04_Cai_Ng/stocks/table(18).csv/vs/1_open?

dbynamed = adply(dbyfile, 1, transform, 
                 vpathren=(strsplit(vpath,"\\\\")[[1]][3]))
ggplot(dbynamed, aes(x=floor(log2(abs(v))))) + geom_histogram(binwidth=1) + 
  facet_wrap(~ vpathren, ncol=1, scales="free") +
  scale_y_continuous("number of values") +
  scale_x_continuous("log base 2 of delta")
ggsave("figs/exponent-distribution.pdf", width=4, height=10)

dbDisconnect(con)