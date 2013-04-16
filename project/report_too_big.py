#
# Copyright (c) 2013, Carnegie Mellon University.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
# HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
# WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

import argparse
import os
import fnmatch
import json
import logging as log
from convertmany import MultiConv

class CountingWriter(object):
	def __init__(self):
		self.reset()

	def reset(self):
		self.nadds = 0
		self.ncols = None

	def add(self, decrow, normalize=True):
		if self.ncols is None:
			self.ncols = len(decrow)
		else:
			if len(decrow) != self.ncols:
				log.error("Inconsistent number of columns on row %s", str(decrow))
		self.nadds += 1

	def done(self, names=None, tsname="timestamps", checked=True):
		pass
		#log.info("done called; nadds,ncols was %d,%d", self.nadds, self.ncols)

	def info(self):
		"""

		Returns:
			(nrow, ncol): number of rows and columns of result
		"""
		return (self.nadds, self.ncols)

def too_big(indir, info, outfp):
	"""
	Args:
		indir: input directory
		info: map from filename patterns to class names
		outfp: stream to write results
	"""
	outfp.write("path,vsize,npoints\n")

	for root, dirs, files in os.walk(indir):
		for (pat, kls) in info.items():
			#get files that match the current pattern
			subset = fnmatch.filter(files, pat)
			#scan each file
			for fname in subset:
				if kls in MultiConv.CLASSES:
					writer = CountingWriter()
					#The count may be off by one row of datapoints if the
					# file has no header row
					conv = MultiConv.CLASSES[kls](writer, header="True")
					log.info("Count %s", fname)
					fpath = os.path.join(root, fname)
					conv.convert(fpath)
					(nrow, ncol) = writer.info()
					for dx in range(ncol):
						outfp.write("%s/vs/%d,>8,%d\n" % (fpath, dx, nrow))
				else:
					log.error("Couldn't find format " + fmt)

if __name__ == "__main__":
	log.basicConfig(level=log.INFO)
	parser = argparse.ArgumentParser(
		description="Aggregate values that are too large for fixed-point encoding")
	parser.add_argument("input", action="store", help="input directory")
	parser.add_argument("infofile", action="store", 
		help="info file; format: map of shell glob patterns to file format")
	parser.add_argument("output", action="store", 
		help="output report file location")

	pargs = parser.parse_args()

	st = None
	with open(pargs.infofile, "r") as fp:
		st = json.load(fp)

	with open(pargs.output, "w") as fp:
		too_big(pargs.input, st, fp)