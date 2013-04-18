library(plyr)
library(ggplot2)
library(RColorBrewer)
library(grid)
library(RSQLite)

setwd(dirname(sys.frame(1)$ofile))
theme_update(plot.margin = unit(c(0,0,0,0), "cm"))

source("common.r")

results = read.results("results/results-nofail.csv")

results.bydset = ddply(results, .(dataset,coder), summarize, 
                       sizemean=mean(sizeorig/sizecomp), 
                       sizesd=sd(sizeorig/sizecomp),
                       decspeed=mean(sizeorig/dectime/1e6),
                       encspeed=mean(sizeorig/enctime/1e6),
                       decspeedsd=sd(sizeorig/dectime/1e6),
                       encspeedsd=sd(sizeorig/enctime/1e6))
results.bycoder = ddply(results, .(coder), summarize, 
                       sizemean=mean(sizeorig/sizecomp), 
                       sizesd=sd(sizeorig/sizecomp),
                       decspeed=mean(sizeorig/dectime/1e6),
                       encspeed=mean(sizeorig/enctime/1e6),
                       decspeedsd=sd(sizeorig/dectime/1e6),
                       encspeedsd=sd(sizeorig/enctime/1e6))

ggplot(results, aes(x=sizeorig/dectime/1e6, y=sizeorig/sizecomp, color=coder)) +
  geom_density2d() +
  scale_y_log10("Uncompressed / compressed ratio (log scale)", breaks=c(1,2,4,8)) +
  scale_x_log10("Decompression rate (MB/s) (log scale)", breaks=c(10,20,40,80)) +
  guides(color=guide_legend(override.aes=list(alpha=1, size=8)))
ggsave("figs/all-results.pdf", width=7, height=5.5)

ggplot(results, aes(x=sizeorig/enctime/1e6, y=sizeorig/sizecomp, color=coder)) +
  geom_density2d() +
  scale_y_log10("Uncompressed / compressed ratio (log scale)", breaks=c(1,2,4,8)) +
  scale_x_log10("Compression rate (MB/s) (log scale)", breaks=c(10,20,40,80)) +
  guides(color=guide_legend(override.aes=list(alpha=1, size=8))) +
  theme(legend.position="none")
ggsave("figs/all-results-encode.pdf", width=4, height=5.5)


ggplot(results.bydset, aes(x=decspeed, y=sizemean, color=coder)) +
  geom_point(size=3) +
  geom_point(data=results.bycoder, size=10) +
  scale_y_log10("Uncompressed / compressed ratio (log scale)", breaks=c(1,2,4,8,16,32)) +
  scale_x_log10("Decompression rate (MB/s) (log scale)", breaks=c(10,20,40,80,160,320)) +
  guides(color=guide_legend(override.aes=list(alpha=1, size=8)))
ggsave("figs/all-results-bydset.pdf", width=7, height=5.5)

ggplot(results.bydset, aes(x=encspeed, y=sizemean, color=coder)) +
  geom_point(size=3) +
  geom_point(data=results.bycoder, size=10) +
  scale_y_log10("Uncompressed / compressed ratio (log scale)", breaks=c(1,2,4,8,16,32)) +
  scale_x_log10("Compression rate (MB/s) (log scale)", breaks=c(10,20,40,80)) +
  guides(color=guide_legend(override.aes=list(alpha=1, size=8))) +
  theme(legend.position="none")
ggsave("figs/all-results-encode-bydset.pdf", width=4, height=5.5)



ggplot(results.bydset, aes(
  x=reorder(dataset,sizemean,mean), 
  y=sizemean, ymin=sizemean-sizesd/4, ymax=sizemean+sizesd/4, color=coder,
  group=coder)) +
  geom_pointrange(size=1) +
  scale_y_log10("Uncompressed / compressed ratio (log scale)", 
                breaks=c(1,2,4,8,16,32,64)) +
  scale_x_discrete("Dataset", breaks=NULL) +
  theme(axis.ticks.x = element_blank(), legend.position="top") +
  coord_cartesian(ylim=c(0.5,100))
ggsave("figs/dataset-sizes.pdf", width=5, height=4)

