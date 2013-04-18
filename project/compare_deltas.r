library(plyr)
library(ggplot2)
library(RColorBrewer)
library(grid)
library(RSQLite)
library(reshape2)

setwd(dirname(sys.frame(1)$ofile))
theme_update(plot.margin = unit(c(0,0,0,0), "cm"))

source("common.r")

results.deltas = read.results("results/results-nofail.csv")
results.deltas$src = "deltas"
results.nodeltas = read.results("results/results-nofail-nodeltas.csv")
results.nodeltas$src = "original"

results.failed = read.results("results/results-failed.csv")
bytesfailed = sum(results.failed$sizeorig)
bytestotal = sum(as.numeric(subset(results.deltas, coder=="elias-gamma")$sizeorig))


all = rbind(results.deltas, results.nodeltas)

bydset = ddply(all, .(dataset,src), summarize, 
               sizemean=mean(sizeorig/sizecomp), 
               sizesd=sd(sizeorig/sizecomp),
               decspeed=mean(sizeorig/dectime/1e6),
               encspeed=mean(sizeorig/enctime/1e6),
               decspeedsd=sd(sizeorig/dectime/1e6),
               encspeedsd=sd(sizeorig/enctime/1e6))

ggplot(bydset, aes(
  x=reorder(dataset,sizemean,mean), 
  y=sizemean, ymin=sizemean-sizesd/2, ymax=sizemean+sizesd/2, color=src,
  group=src)) +
  geom_point(size=4, position="dodge") +
  scale_y_log10("Uncompressed / compressed ratio (log scale)", 
                breaks=c(1,2,4,8,16,32,64)) +
  scale_x_discrete("Dataset", breaks=NULL) +
  theme(axis.ticks.x = element_blank(), legend.position="top") +
  coord_cartesian(ylim=c(0.5,100)) +
  theme(legend.text=element_text(size=18))
ggsave("figs/deltas-dataset-sizes.pdf", width=5, height=4)

byenc = ddply(all, .(coder,src), summarize,
              sizemean=mean(sizeorig/sizecomp), 
              sizesd=sd(sizeorig/sizecomp))

ggplot(byenc, aes(
  x=coder, 
  y=sizemean,
  fill=src)) +
  geom_bar(stat="identity", position="dodge") +
  scale_y_continuous("Uncompressed / compressed ratio") +
  scale_x_discrete("Encoder") +
  theme(axis.ticks.x = element_blank(), legend.position="top") +
  theme(legend.text=element_text(size=18))
ggsave("figs/deltas-coder-sizes.pdf", width=5, height=4)