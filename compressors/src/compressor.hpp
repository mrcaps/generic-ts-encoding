/*
 * compressor.hpp
 * @brief get compressors
 * @author ishafer
 */

#ifndef COMPRESSOR_HPP_
#define COMPRESSOR_HPP_

#include <unistd.h>
#include <cstdlib>

#include "compressor/coder.hpp"
#include "compressor/bitstream.hpp"
#include "compressor/delta.hpp"
#include "compressor/eliasgamma.hpp"
#include "compressor/eliasdelta.hpp"
#include "compressor/loghuffman.hpp"
#include "compressor/zigzag.hpp"
#include "compressor/zlib.hpp"

#include "../inc/timer.h"

#include "metastore.hpp"

using namespace std;

enum CoderName {
	ELIAS_GAMMA,
	ELIAS_DELTA,
	LOG_HUFFMAN,
	ZLIB
};

ostream& operator<<(ostream& os, const CoderName& coder)
{
	switch(coder) {
		case ELIAS_GAMMA: os << "elias-gamma"; break;
		case ELIAS_DELTA: os << "elias-delta"; break;
		case LOG_HUFFMAN: os << "log-huffman"; break;
		case ZLIB: os << "zlib"; break;
	}
	return os;
}

template<typename vT, typename bsT>
const Coder<vT, bsT> *get_coder(CoderName name) {
	switch (name) {
	case ELIAS_GAMMA:
		return new EliasGamma<vT, bsT>;
	case ELIAS_DELTA:
		return new EliasDelta<vT, bsT>;
	case LOG_HUFFMAN:
		return new LogHuffman<vT, bsT>;
	case ZLIB:
		return new ZLib<vT, bsT>;
	default:
		cerr << "Unknown coder type:" << name << endl;
		return new EliasGamma<vT, bsT>;
	}
}

template<typename vT, typename bsT>
void test_roundtrip_inner(CoderName name, void *asbytes, uint64_t npoints) {
	//npoints = 20;
	const Coder<vT, bsT> *coder = get_coder<vT, bsT>(name);

	Timer timer;

	delta_enc_inplace((vT*) asbytes, npoints);

	unsigned char *outbits = (unsigned char*) calloc(npoints*BUF_SCALE_FACTOR+12, sizeof(vT));
	uint64_t orig_outsize = sizeof(vT)*(npoints*BUF_SCALE_FACTOR+12);
	uint64_t outsize = orig_outsize;
	uint64_t insize = npoints;
	outbits = coder->enc(outbits, &outsize, (vT*) asbytes, insize);

	double tenc = timer.elapsed();
	timer.restart();

	vT* dout = (vT*) malloc(sizeof(vT)*(npoints*BUF_SCALE_FACTOR+12));
	dout = coder->dec(dout, &insize, outbits, orig_outsize);

	double tdec = timer.elapsed();

	cout << name << "," << npoints*sizeof(vT) << "," << outsize << "," << tenc << "," << tdec;

	//print_arr((T*) asbytes, npoints);
	//cout << endl;
	//print_arr(dout, npoints);
	if ( 0 == memcmp(dout, asbytes, npoints*sizeof(vT)) ) {
		cout << endl;
	} else {
		cout << "FAIL" << endl;
	}

	free(outbits);
	free(dout);
	delete coder;
}

void test_roundtrip(CoderName name, vstream vs) {
	void *bytes = read_fully(vs);
	//surely there must be a cleaner way of doing this?
	switch (vs.vsize) {
	case 1:
		test_roundtrip_inner<int8_t, uint8_t>(name, bytes, vs.npoints);
		break;
	case 2:
		test_roundtrip_inner<int16_t, uint16_t>(name, bytes, vs.npoints);
		break;
	case 4:
		test_roundtrip_inner<int32_t, uint32_t>(name, bytes, vs.npoints);
		break;
	case 8:
		test_roundtrip_inner<int64_t, uint64_t>(name, bytes, vs.npoints);
		break;
	default:
		cerr << "Unknown value size:" << vs.vsize << endl;
		break;
	}
}

void test_roundtrips() {
	vector<vstream> streams = get_test_streams();
	for (vector<vstream>::iterator it = streams.begin(); it != streams.end(); ++it) {
		test_roundtrip(ELIAS_GAMMA, *it);
		test_roundtrip(ELIAS_DELTA, *it);
		test_roundtrip(LOG_HUFFMAN, *it);
		test_roundtrip(ZLIB, *it);
	}
}

void test_compressor()  {
	//bitstream
	test_bitstream();

	//delta
	test_delta_basic();
	test_delta_overflow();

	//eliasgamma
	test_elias_gamma();

	//eliasdelta
	test_elias_delta();

	//loghuffman
	test_loghuffman();

	//zigzag
	test_zigzag();

	//zlib
	test_zlib();

	//roundtrips
	test_roundtrips();
}

#endif /* COMPRESSOR_HPP_ */
