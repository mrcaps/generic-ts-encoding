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
Convert an individual data file to timestamp/value columns
Use fixed-point representation; 

Note: requires py3k
"""

import unittest
import sys
import os
import posixpath
path_joiner = posixpath.join

import shutil
from array import array
import math
import string
import logging as log
from decimal import Decimal, ROUND_UP
import sqlite3

import datetime
import time
import calendar

import struct

try:
	array("q")
except:
	raise EnvironmentError("8-byte array not supported; be sure to use py3k")

#variable-width sizes
ITEMSIZES = {
	1: "b",
	2: "h",
	4: "l",
	8: "q"
}
MAX_ITEMSIZE = 0
for (k, v) in ITEMSIZES.items():
	assert array(v).itemsize == k
	ITEMSIZES[k] = (k, v)
	MAX_ITEMSIZE = max(MAX_ITEMSIZE, k)
_last_itemsize = None
for i in range(MAX_ITEMSIZE, 0, -1):
	if i in ITEMSIZES:
		_last_itemsize = ITEMSIZES[i]
	else:
		ITEMSIZES[i] = _last_itemsize


def get_value_size(val):
	"""
	Args:
		val: an integer
	"""
	return int(math.ceil((int.bit_length(val)+1)/8))


class MetaStore(object):
	"""Stream metadata storage.
	"""
	def __init__(self, fpath):
		self.fpath = fpath
		self.conn = None
		self.open()
		#TODO: vpath is the primary key...
		self.conn.execute("""CREATE TABLE IF NOT EXISTS meta 
			(vname text, 
				tpath text, vpath text primary key, 
				tmin integer, tmax integer, tscale integer, tsize integer,
				vmin integer, vmax integer, vscale integer, vsize integer, 
				npoints integer)""")
		self.conn.commit()
		self.conn.close()

	def open(self):
		self.conn = sqlite3.connect(self.fpath)

	def close(self):
		self.conn.close()
		self.conn = None

	def addMS(self, relpath, ms):
		"""Add another metadata store to this one.

		Args:
			relpath: the relative path to the other MetaStore
			ms: the other MetaStore
		"""
		oldrf = ms.conn.row_factory

		ms.conn.row_factory = sqlite3.Row
		cur = ms.conn.cursor()
		cur.execute("SELECT * from meta")
		for row in cur:
			cols = row.keys()
			query = "INSERT INTO meta (%s) VALUES (%s)" % (
				",".join(cols), ",".join(("?",) * len(cols))
			)
			self.conn.execute(query, [row[col] for col in cols])

		self.conn.commit()
		ms.conn.row_factory = oldrf

	def add(self, vname, tpath, vpath, 
		tmin, tmax, tscale, tsize,
		vmin, vmax, vscale, vsize, 
		npoints):
		"""Add a stream to the metadata store.
		
		Pre: MetaStore must be opened.
		Post: MetaStore is not closed.
		"""
		self.conn.execute(
			"""INSERT INTO meta VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)""",
			(vname, tpath, vpath,
			tmin, tmax, tscale, tsize, 
			vmin, vmax, vscale, vsize, 
			npoints))
		self.conn.commit()

class Writer(object):
	"""A binary column writer."""
	TS = True
	VS = False

	def __init__(self, rootdir):
		self.tsdir = path_joiner(rootdir, "ts")
		self.vsdir = path_joiner(rootdir, "vs")
		for d in [self.tsdir, self.vsdir]:			
			try:
				os.makedirs(d)
			except OSError:
				pass
		self.meta = MetaStore(os.path.join(rootdir, "meta.db"))

		self.reset()

	def reset(self):
		self.started = False
		#minimum exponents so far in this writer.
		# use this to push everything to integral values
		self.minexps = None
		self.vals = []
		self.prevts = None

	def add(self, decrow, normalize=True):
		"""
		Args:
			ts: Decimal timestamp
			normalize: should we trim extra zeros off the end of the digits?
			drow: a row of Decimals. First is the timestamp.
		"""
		if normalize:
			decrow = [d.normalize() for d in decrow]
		dtups = [d.as_tuple() for d in decrow]

		if not self.started:
			self.minexps = [d.exponent for d in dtups]
			self.maxexps = [d.exponent for d in dtups]
			self.maxabsv = [d for d in decrow]
			#maximum representation value size

			self.started = True

		self.minexps = list(map(min, self.minexps, [d.exponent for d in dtups]))
		self.maxexps = list(map(max, self.maxexps, [d.exponent for d in dtups]))
		self.maxabsv = list(map(max, self.maxabsv, [abs(d) for d in decrow]))
		self.vals.append(decrow)

	def reverse(self):
		"""Reverse values in time."""
		self.vals.reverse()

	def done(self, names=None, tsname="timestamps", checked=True):
		"""Write out output.
		minexps are now the scale factors for columns

		Args:
			names: column names
			checked: run consistency checks
		"""
		arrs = []
		arrsizes = []
		for i in range(len(self.vals[0])):
			maxd = self.maxabsv[i].scaleb(-self.minexps[i])
			exp = get_value_size(int(maxd))
			if exp > MAX_ITEMSIZE:
				log.info("converted max int was %d", maxd)
				log.info("min exponent was %d", self.minexps[i])
				raise ValueError("value too large to represent: " + str(self.maxabsv[i]))
			(size, name) = ITEMSIZES[exp]
			arrs.append(array(name))
			#acutal encoded size
			arrsizes.append(size)
		for rdx in range(len(self.vals)):
			dt = self.vals[rdx]
			for i in range(len(dt)):
				#scale out to minimum exponent
				scaled = dt[i].scaleb(-self.minexps[i])
				if checked: 
					assert scaled == scaled.to_integral_value()
				arrs[i].append(int(scaled))

		if names is None:
			names = [str(k) for k in range(len(arrs))]
		else:
			#remove non-letters from names
			names = [str(k) + "_" + "".join([c for c in name.lower() if 
					(c in string.ascii_lowercase or c in string.digits)])
				for (k, name) in zip(range(len(names)), names)]

			
		if len(names) < len(arrs):
			log.error("Name length: %d", len(names))
			log.error("Value length: %d", len(arrs))
			raise ValueError("Name length was less than array length")

		self.meta.open()
		for (dx, arr, name, minexp) in zip(range(len(arrs)), arrs, names, self.minexps):
			vspath = path_joiner(self.vsdir, name)
			writepath = vspath
			if dx == 0:
				tspath = path_joiner(self.tsdir, name)
				mints = min(arr)
				maxts = max(arr)
				writepath = tspath
				if checked:
					if len(arr) > 1 and arr[1] - arr[0] <= 0:
						assert False, "timestamp_1 - timestamp_0 is nonpositive"
					lastts = arr[0] - 1 
					for it in arr:
						assert it > lastts, "monotonically increasing timestamps at " + str(it)
					#print("minmax_ts", min(arr), max(arr))

			else:
				self.meta.add(name, tspath, vspath, 
					mints, maxts, self.minexps[0], arrsizes[0],
					min(arr), max(arr), minexp, arrsizes[dx],
					len(arr))

			#write out values to file and metastore
			with open(writepath, "wb") as fp:
				arr.tofile(fp)

		self.meta.close()
		self.reset()

class Converter(object):
	def __init__(self, writer):
		self.writer = writer

	def convert(self, fname):
		log.error("override me.")

def datetime_to_timestamp(dt):
	return int(calendar.timegm(dt.timetuple()))

class MulticolumnTextConverter(Converter):
	"""
	Takes keywords:
		timestamps: type of timestamp
			"fmt:<format string>" (parse according to <format string>)
			"epoch" (same as numeric)
			"numeric" (write literal integer)
			"skip" (ignore first column [e.g., for unique id])
		maxyear: for two-year dates before 1969: what is the maximum possible date?
		header: does the file have a header? (true/false)
		header_rows: how many header rows are there?
		header_names: manually define a header, instead of reading the last row
		quantize: for floats that were written to file, quantize to this value
			e.g., "0.000001"
		reverse: should we reverse the file order?
	"""
	def __init__(self, writer, **kwds):
		#assume no timestamp column if unspecified
		self.timestamps = kwds.get("timestamps", "none")
		self.header = kwds.get("header", "False").lower() == "true"
		self.header_rows = int(kwds.get("header_rows", "1"))
		self.header_names = kwds.get("header_names", None)
		self.sep = kwds.get("sep", None)
		self.reverse = kwds.get("reverse", "False").lower() == "true"
		self.maxyear = int(kwds.get("maxyear", 10000))
		self.quantize = kwds.get("quantize", None)
		super(MulticolumnTextConverter, self).__init__(writer)

	def convert(self, fname):
		#column names
		names = None

		with open(fname, "r") as fp:
			lno = -1
			for line in fp:
				line = line.rstrip()

				#TODO: what to do about NaNs?
				if line.find("NaN") > -1:
					log.warning("Got NaN on line " + line.strip())
					line = line.replace("NaN", "0")

				#skip empty lines
				if len(line) == 0:
					continue
				lno += 1
				if self.header and lno < self.header_rows:
					names = line.split(self.sep)
					continue
				splt = line.split(self.sep)
				ts = Decimal(lno)
				if self.timestamps != "none":
					if self.timestamps.startswith("fmt:"):
						fmt = self.timestamps[len("fmt:"):]
						dt = datetime.datetime.strptime(splt[0], fmt)
						if dt.year > self.maxyear:
							dt = dt.replace(dt.year - 100)
						ts = Decimal(datetime_to_timestamp(dt))
					elif self.timestamps == "year":
						dt = datetime.date(int(splt[0]), 1, 1)
						ts = Decimal(datetime_to_timestamp(dt))
					elif self.timestamps == "epoch":
						#do we want any way of marking epoch vs numeric?
						ts = Decimal(splt[0])
					elif self.timestamps == "numeric":
						ts = Decimal(splt[0])
					elif self.timestamps == "skip":
						#some non-time identifier in the first column
						pass

					splt = splt[1:]
				
				row = [Decimal(d) for d in splt]
				if self.quantize is not None:
					qdec = Decimal(self.quantize)
					row = [r.quantize(qdec) for r in row]
				self.writer.add([ts] + row)

		if self.reverse:
			self.writer.reverse()

		if self.header_names is not None:
			names = self.header_names
		self.writer.done(names=names)

def filtered_decimal(st):
	"""Filter a few errors from input data
	"""
	if st.find("e") > -1:
		#where are these bogus tiny raw values coming from?
		log.warning("Bogus value: " + st)
		return Decimal(0)		
	return Decimal(st) 

class SkeletonAnimationFileConverter(Converter):
	"""
	Takes keywords:
		names: channel names (list)
	"""
	def __init__(self, writer, **kwds):
		self.names = kwds.get("names", None)
		if self.names is None:
			self.colnames = None
		else:
			colnames = [[n+"_x", n+"_y", n+"_z"] for n in self.names]
			self.colnames = [item for sublist in colnames for item in sublist]
		super(SkeletonAnimationFileConverter, self).__init__(writer)	

	def convert(self, fname):
		with open(fname, "r") as fp:
			curblock = dict()

			for line in fp:
				#skip comments
				if line.startswith("//"):
					continue
				
				if line.startswith(" ") or line.startswith("\t"):
					#XXX: what to do about NaNs?
					if line.find("-1.#IND") > -1:
						log.warning("Got #IND on line" + line.strip())
						line = line.replace("-1.#IND", "0")

					parts = line.lstrip().split()
					assert len(parts) == 4
					chan = parts[0]
					#print("p", parts)
					try:
						decs = [filtered_decimal(d) for d in parts[1:]]
					except:
						log.error("Inconvertible line " + line.strip())
						raise
					curblock[int(chan)] = decs
				else:
					#next timestamp line
					try:
						ts = Decimal(line)
					except:
						print("Inconvertible timestamp " + line.strip())
						raise
					if len(curblock) > 0:
						row = []
						for chan in sorted(curblock.keys()):
							row.extend(curblock[chan])
						self.writer.add([ts] + row)
						curblock = dict()

		self.writer.done(names=(["timestamps"] + self.colnames))

assert array('d').itemsize == 8

class HeaderBinaryConverter(Converter):
	def __init__(self, writer, **kwds):
		"""
		Takes keywords:
			datatype: type code for data (default 'd' for 8-byte double)
			nstreams: *required* number of streams
			streamlen: *required* length of each stream
		"""
		self.writer = writer
		self.datatype = kwds.get("datatype", "d")
		self.nstreams = int(kwds.get("nstreams", "1"))
		self.streamlen = int(kwds.get("streamlen", "1"))
		self.quantize = kwds.get("quantize", None)

	def convert(self, fname):
		#stream addresses
		addresses = []
		#stream contents
		arrs = []

		with open(fname, "r+b") as fp:
			hdxsize = struct.calcsize("i")
			assert hdxsize == 4, "integers were not size 4!"
			addsize = struct.calcsize("l")
			assert addsize == 4, "integers were not size 4!"

			#read header. Assume 
			for i in range(self.nstreams):
				streamno = struct.unpack("i", fp.read(hdxsize))[0]
				assert streamno == i, "expect stream number to increase by 1"
				addresses.append(struct.unpack("l", fp.read(addsize))[0])

			for i in range(self.nstreams):
				fp.seek(addresses[i])
				arr = array("d")
				arr.fromfile(fp, self.streamlen)
				arrs.append(arr)

		for i in range(self.streamlen):
			row = [Decimal(arr[i]) for arr in arrs]
			if self.quantize is not None:
				qdec = Decimal(self.quantize)
				row = [r.quantize(qdec) for r in row]
			self.writer.add([Decimal(i)] + row)

		self.writer.done()

class MATLABConverter(Converter):
	def __init__(self, writer, **kwds):
		super(MATLABConverter, self).__init__(writer)	

	def convert(self, fname):
		pass

class TestParse(unittest.TestCase):
	@classmethod
	def setUpClass(cls):
		try:
			shutil.rmtree("tmp")
		except OSError:
			pass

	def test_multicol_text(self):
		conv = MulticolumnTextConverter(Writer("tmp/test_multicol_text"))
		conv.convert("testdata/phone1.head.txt")

	def test_multicol_text2(self):
		conv = MulticolumnTextConverter(Writer("tmp/test_multicol_text2"))
		conv.convert("testdata/robot_arm_1.dat")

	def test_skeleton_animation_file(self):
		conv = SkeletonAnimationFileConverter(Writer("tmp/test_skeleton"),
			names=["name%d" % (d) for d in range(15)])
		conv.convert("testdata/Vi_Chute_cote_pos.san")

	def test_stocks(self):
		conv = MulticolumnTextConverter(Writer("tmp/stocks"),
			header="True", timestamps="fmt:%d-%b-%y", sep=",", reverse="True")
		conv.convert("testdata/stocks.head.csv")

		conv = MulticolumnTextConverter(Writer("tmp/stocks70"),
			header="True", timestamps="fmt:%d-%b-%y", sep=",", reverse="True", maxyear="2020")
		conv.convert("testdata/stocks.70.csv")

	def test_ecg(self):
		conv = MulticolumnTextConverter(Writer("tmp/ecg"),
			header="True", header_rows="2", timestamps="numeric")
		conv.convert("testdata/superven1.txt")

	def test_singlecol(self):
		conv = MulticolumnTextConverter(Writer("tmp/ca_income"), timestamps="none")
		conv.convert("testdata/CAincome.txt")

	def test_get_value_size(self):
		self.assertEqual(1, get_value_size(-8))
		self.assertEqual(1, get_value_size(7))

		self.assertEqual(1, get_value_size(127))
		self.assertEqual(2, get_value_size(128))
		self.assertEqual(2, get_value_size(32767))
		self.assertEqual(3, get_value_size(32768))
		self.assertEqual(4, get_value_size(2147483647))
		self.assertEqual(5, get_value_size(2147483648))
		self.assertEqual(8, get_value_size(9223372036854775807))
		self.assertEqual(9, get_value_size(9223372036854775808))

		self.assertEqual(2, get_value_size(-32767))
		self.assertEqual(3, get_value_size(-40000))

	def test_negative_time(self):
		dt = datetime.datetime.strptime("01-Oct-03", "%d-%b-%y")
		self.assertEqual(datetime_to_timestamp(dt), 1064966400)

		dt = datetime.datetime.strptime("01-Jan-70", "%d-%b-%y")
		self.assertEqual(datetime_to_timestamp(dt), 0)

		dt = datetime.datetime.strptime("02-Jan-70", "%d-%b-%y")
		self.assertEqual(datetime_to_timestamp(dt), 86400)

		dt = datetime.datetime.strptime("31-Dec-69", "%d-%b-%y")
		self.assertEqual(datetime_to_timestamp(dt), -86400)

	def test_header_binary(self):
		#actual streamlen is 3000; use 300 for a shorter test.
		conv = HeaderBinaryConverter(Writer("tmp/binary"), 
			nstreams=93, streamlen=300, quantize="0.01") #stocks.dat

		#	nstreams=12218, streamlen=962, quantize="0.1") #tao.dat
		conv.convert("testdata/stocks.dat")
		
if __name__ == "__main__":
	if len(sys.argv) == 1:
		unittest.main()
		sys.exit(1)

	parser = argparse.ArgumentParser(description="Convert datafile to ts/vs")
	parser.add_argument("format", help="input format")
	parser.add_argument("input", action="store", help="input file")
	parser.add_argument("output", action="store", help="output target")

	pargs = parser.parse_args()