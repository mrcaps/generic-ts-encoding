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

"""
Convert a folder of data based on a json spec.

Note: requires py3k
"""

from convert import Writer
from convert import MulticolumnTextConverter
from convert import SkeletonAnimationFileConverter
from convert import HeaderBinaryConverter
import glob
import json
import unittest
import sys
import shutil
import logging as log
import os
import argparse

class MultiConv(object):
	CLASSES = {
		"MulticolumnText": MulticolumnTextConverter,
		"SkeletonAnimationFile": SkeletonAnimationFileConverter,
		"HeaderBinaryFile": HeaderBinaryConverter
	}

	def __init__(self, infofile, outdir, skip_existing=True):
		#change to the info file's path
		prevwd = os.getcwd()
		(fdir, fname) = os.path.split(infofile)
		with open(infofile, "r") as fp:	
			desc = json.load(fp)
			for di in desc:
				files = []
				if "paths" in di:
					paths = di["paths"]
					os.chdir(fdir)
					for p in paths:
						files += glob.glob(p)
					os.chdir(prevwd)
				else:
					log.warning("Missing paths in" + str(desc))
					continue

				args = dict()
				if "args" in di:
					args = di["args"]

				if "format" in di:
					fmt = di["format"]
					if not fmt in self.CLASSES:
						log.error("Couldn't find format " + fmt)
					for name in files:
						fulloutdir = os.path.join(outdir, name)
						if skip_existing and os.path.exists(fulloutdir):
							log.info("\tSkipping completed %s", name)
							continue
						log.info("\tConverting %s", name)
						writer = Writer(fulloutdir)
						conv = self.CLASSES[fmt](writer, **args)
						conv.convert(os.path.join(fdir, name))

class TestInfo(unittest.TestCase):
	def setUp(self):
		try:
			shutil.rmtree("tmp")
		except OSError:
			pass

	def test_info(self):
		mc = MultiConv("testdata/Population/info.json", "tmp/multiconv")

def convert_all(indir, outdir, infoname="info.json", skip_existing=False):
	"""
	Args:
		indir: input directory
	"""
	for root, dirs, files in os.walk(indir):
		if infoname in files:
			fullinfopath = os.path.join(root, infoname)
			fulloutdir = os.path.join(outdir, os.path.relpath(root, indir))
			if skip_existing and os.path.exists(fulloutdir):
				log.info("Skipping completed %s", fullinfopath)
				continue
			log.info("Converting %s -> %s", fullinfopath, fulloutdir)
			MultiConv(fullinfopath, fulloutdir)


if __name__ == "__main__":
	if len(sys.argv) == 1:
		unittest.main()
		sys.exit(1)

	log.basicConfig(level=log.INFO)
	parser = argparse.ArgumentParser(description="Convert a directory to ts/vs")
	parser.add_argument("input", action="store", help="input directory")
	parser.add_argument("output", action="store", help="output directory")

	pargs = parser.parse_args()

	convert_all(pargs.input, pargs.output)