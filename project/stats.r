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

#get npoints information
if (!exists("npts")) {
  npts = dbGetQuery(con, paste("select vsize, npoints from meta"))
  toobig = read.csv(paste(outdir, "toobig.csv", sep="/"))[,c("vsize","npoints")]
  data = rbind(npts, toobig)
}

horizplot = function(data, title) {
  ggplot(data, aes(
    x=factor(1), 
    y=val,
    fill=factor(vsize, levels=c("1","2","4","8",">8"))
  )) + 
    geom_bar(width=1, stat="identity") + coord_flip() +
    scale_fill_brewer(palette="Blues", name="Value size\n(bytes)") +
    scale_y_continuous(title) + 
    scale_x_discrete(breaks=c()) +
    theme(axis.title.y=element_blank(),
          panel.background=element_blank())
}

rotate1 = function(df) {
  #rotate data frame by one row
  rbind(df[2:nrow(df),], df[1,])
}

bystreams = ddply(data, .(vsize), summarize, val=length(vsize))
horizplot(rotate1(bystreams), "Number of Streams")
ggsave("figs/value-size-bystreams.pdf", width=5.5, height=2)

bypoints = ddply(data, .(vsize), summarize, val=sum(npoints))
horizplot(rotate1(bypoints), "Number of Values")
ggsave("figs/value-size-bypoints.pdf", width=5.5, height=2)


dbDisconnect(con)