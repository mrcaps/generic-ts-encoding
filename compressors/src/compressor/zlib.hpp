/*
 * zlib.hpp
 *
 *  Created on: Apr 11, 2013
 *      Author: ishafer
 */

#ifndef ZLIB_HPP_
#define ZLIB_HPP_

#include "coder.hpp"
#include "bitstream.hpp"
#include "zigzag.hpp"
#include "../util.hpp"

#include <iostream>
#include <cstring>
#include <cassert>

using namespace std;

#include "../../inc/zlib.h"
#define byte Bytef

/**
 * Elias-gamma encoding with zig-zag first.
 * May leave some garbage bits at the end of the output.
 */
template<typename vT, typename bsT>
class ZLib : public Coder<vT, bsT> {
public:
	ZLib() {};
	~ZLib() {};

	unsigned char* enc(
			unsigned char *out,
			uint64_t *outsize,
			vT *in,
			uint64_t insize) const {
		uLongf l_outsize = *outsize;
		//cout << "enc outsize before=" << *outsize << endl;
		int rc = compress(out,
				&l_outsize,
				reinterpret_cast<byte*>(in),
				insize*sizeof(vT));
		*outsize = l_outsize;
		//cout << "enc outsize after=" << *outsize << endl;
		if (rc != Z_OK) {
			cout << "ERROR: ZLib fail; retcode=" << rc << endl;
		}
		return out;
	}

	vT* dec(vT *out,
			uint64_t *outsize,
			unsigned char *in,
			uint64_t insize) const {
		uLongf l_outsize = insize*sizeof(vT)*BUF_SCALE_FACTOR+12; //XXX: need to pass this in-band
		//cout << "dec outsize before=" << *outsize << "," << insize << endl;
		int rc = uncompress(reinterpret_cast<byte*>(out),
				&l_outsize,
				in,
				insize*sizeof(vT));
		*outsize = l_outsize;
		//cout << "dec outsize after=" << *outsize << endl;
		if (rc != Z_OK) {
			cout << "ERROR: ZLib fail; retcode=" << rc << endl;
		}
		return out;
	}

private:
	DISALLOW_EVIL_CONSTRUCTORS(ZLib);
};

void test_zlib_unwrapped() {
	vstream vs = get_test_stream();
	byte *data = (byte*) read_fully(vs);

	//cout << ((int32_t*) data)[0] << endl;
	//cout << ((int32_t*) data)[1] << endl;

	uLongf dlen = vs.npoints*vs.vsize;
	//cout << "data length " << dlen<< endl;
	//assert( 0 == memcmp(data, data, dlen) );

	uLongf buflen = vs.npoints*vs.vsize*1.01+12;
	//cout << "old buflen " << buflen << endl;
	byte *compressed = static_cast<byte*>(malloc(buflen));

	assert( Z_OK == compress(compressed, &buflen, data, dlen) );

	//cout << "new buflen " << buflen << endl;

	byte *uncompressed = static_cast<byte*>(malloc(dlen));
	uLongf newlen = dlen;
	assert( Z_OK == uncompress(uncompressed, &newlen, compressed, buflen) );

	//cout << "uncompressed length " << newlen << endl;

	assert( 0 == memcmp(data, uncompressed, dlen) );
}

void test_zlib_basic() {
	ZLib<int32_t, uint32_t> coder32;
	ZLib<int64_t, uint64_t> coder64;

	int32_t din[] = {1, 2, 4, 5, 6, -3, 8};
	test_coder_array(coder32, (int32_t*) din, sizeof(din)/sizeof(int32_t));

	int32_t din2[] = {0, 181817, 363636, 545454, 363636, 363636, 545454, 1, 2, 3, 4, 5};
	test_coder_array(coder32, (int32_t*) din2, sizeof(din2)/sizeof(int32_t));

	int64_t din3[] = {31014740000, 31000620000, 30985390000, 30968450000, 30950330000};
	test_coder_array(coder64, (int64_t*) din3, sizeof(din3)/sizeof(int64_t));
}

void test_zlib() {
	test_zlib_unwrapped();
	test_zlib_basic();
}

#endif /* ZLIB_HPP_ */
