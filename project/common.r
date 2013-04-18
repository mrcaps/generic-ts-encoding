
read.results = function(path) {
  results = read.csv(path, header=FALSE, sep=",",
           col.names=c("vname","tpath","vpath","vsize",
                       "coder","sizeorig","sizecomp","enctime","dectime"),
           stringsAsFactors=FALSE)
  results$dataset = sapply(results$vpath, function(x) strsplit(x,"\\\\")[[1]][2])
  results
}