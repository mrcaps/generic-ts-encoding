/**
 * eliasgamma.hpp
 * Elias-Gamma encoder/decoder
 *
 * @author ishafer
 */

#ifndef ELIASGAMMA_HPP_
#define ELIASGAMMA_HPP_

#include <iostream>

#include "coder.hpp"
#include "bitstream.hpp"
#include "zigzag.hpp"
#include "../util.hpp"

using namespace std;

/**
 * Elias-gamma encoding with zig-zag first.
 * May leave some garbage bits at the end of the output.
 */
template<typename vT, typename bsT>
class EliasGamma : public Coder<vT, bsT> {
public:
	EliasGamma() {};
	~EliasGamma() {};

	unsigned char* enc(
			unsigned char *out,
			uint64_t *outsize,
			vT *in,
			uint64_t insize) const {
		BitStream<unsigned char, bsT> bs(out, *outsize, WRITE);
		for (uint64_t i = 0; i < insize; ++i) {
			uint64_t v = ZIGZAG_ENC(static_cast<int64_t>(in[i])) + 1;
			int32_t nb = nbits(v);
			//cout << "in[" << i << "]=" << v << " (" << nb << " bits)" << endl;
			bs.write_bits(0, nb-1);
			bs.write_bits(v, nb);
		}
		*outsize = bs.written_size();

		return bs.get_backing();
	}

	vT* dec(vT *out,
			uint64_t *outsize,
			unsigned char *in,
			uint64_t insize) const {
		BitStream<unsigned char, bsT> bs(in, insize, READ);
		for (uint64_t i = 0; i < *outsize; ++i) {
			uint32_t nb = 0;
			while (bs.ready() && (!bs.read_bit())) {
				++nb;
			}
			uint64_t b = bs.read_bits(nb);
			uint64_t v = (b | ((bsT) 1 << nb)) - 1;
			//cout << "got " << nb << " bits ->" << (uint64_t) v << endl;
			out[i] = ZIGZAG_DEC(v);
		}
		return out;
	}

private:
	DISALLOW_EVIL_CONSTRUCTORS(EliasGamma);
};

void test_log2() {
	assert( nbits((uint64_t) 17) == 5 );
	assert( nbits((uint32_t) 17) == 5 );
	assert( nbits((uint16_t) 17) == 5 );
	assert( nbits((uint8_t) 17) == 5 );
	assert( nbits(17) == 5 );
}

void test_elias_gamma_single() {
	EliasGamma<int32_t, uint32_t> coder32;

	int32_t din1[] = {0,-1,1,-2,2,-3,3};
	unsigned char out[20];
	memset(&out, 0, sizeof(out));
	uint64_t outsize = sizeof(out);
	uint64_t insize = sizeof(din1)/sizeof(int32_t);
	coder32.enc(static_cast<unsigned char*>(out),
			&outsize, (int32_t*) din1, insize);

	int32_t* dout = static_cast<int32_t*>(malloc(sizeof(din1)));
	coder32.dec(dout, &insize, out, insize);

	assert( 0 == memcmp(dout, din1, sizeof(din1)) );
}

void test_elias_gamma_basic() {
	EliasGamma<int8_t, uint8_t> coder8;
	EliasGamma<int32_t, uint32_t> coder32;
	EliasGamma<int64_t, uint64_t> coder64;

	int32_t din[] = {1, 2, 4, 5, 6, -3, 8};
	//zigzag encoded =>
	//	2, 4, 8, 10, 12, 5, 16
	// +1 =>
	//	3, 5, 9, 11, 13, 6, 17
	test_coder_array(coder32, (int32_t*) din, sizeof(din)/sizeof(int32_t));

	int32_t din2[] = {0, 181817, 363636, 545454, 363636, 363636, 545454, 1, 2, 3, 4, 5};
	test_coder_array(coder32, (int32_t*) din2, sizeof(din2)/sizeof(int32_t));

	int64_t din3[] = {31014740000, 31000620000, 30985390000, 30968450000, 30950330000};
	test_coder_array(coder64, (int64_t*) din3, sizeof(din3)/sizeof(int64_t));

	int8_t din4[] = {
			-17, -54, -68, -63, -81, -43, -54, -76, -47, -49, -21, -54, -51, -36, 13, -13,
			-23, -6, 31, 23, 2, 9, 41, 60, 46, 24, 32, 73, 47, 38, 42, -17, -68, -17, -17,
			-43, -54, -17, -47, -17, -17, -17, -17, -36, 13, -17, -17, -6, -17, 23, 2, -17,
			41, 60, 46, 24, -17, 73, 47, 38, -17, -68, -54, -81, -43, -54, -54, -47, -54,
			-54, -54, -54, -36, -54, -54, -54, -6, 31, 23, 2, -54, 41, -54, 46, -54, 32,
			-54, -54, -54, 42, -68, -68, -68, -68, -68, -47, -49, -21, -68, -68, -36, -68,
			-68, -23, -68, -68, 23, -68, -68, -68, -68, -68, -68, 32, -68, 47, 38, -68, -63,
			-43, -54, -63, -47, -49, -63, -54, -51, -36, -63, -63, -23, -63, 31, 23, 2, -63,
			-63, -63, -63, 24, 32, -63, -63, 38, -63, -81, -54, -81, -47, -49, -21, -81,
			-51, -81, -81, -81, -81, -6, 31, 23, -81, 9, -81, -81, 46, -81, 32, 73, -81,
			-81, 42, -43, -43, -47, -49, -43, -43, -51, -43, 13, -13, -23, -6, 31, 23, -43,
			9, 41, -43, -43, 24, 32, 73, -43, 38, 42, -54, -54, -54, -21, -54, -51, -36, 13,
			-54, -54, -54, 31, -54, 2, 9, -54, -54, 46, 24, -54, 73, -54, -54, 42, -47, -49,
			-21, -76, -51, -76, 13, -13, -76, -6, 31, 23, 2, 9, -76, 60, 46, 24, -76, -76,
			47, 38, -76, -49, -47, -47, -47, -47, 13, -13, -47, -6, -47, -47, -47, 9, 41,
			60, 46, 24, -47, 73, -47, 38, 42, -21, -54, -49, -36, 13, -13, -49, -49, -49,
			-49, 2, 9, -49, -49, 46, 24, 32, -49, -49, -49, -49, -54, -21, -21, 13, -21,
			-21, -21, 31, -21, -21, 9, -21, 60, -21, -21, 32, 73, -21, -21, 42, -51, -54,
			-54, -54, -54, -54, 31, 23, -54, -54, -54, -54, 46, 24, -54, 73, 47, -54, -54,
			-51, -51, -51, -23, -6, -51, 23, -51, 9, -51, -51, -51, -51, -51, 73, -51, -51,
			42, -36, -36, -36, -6, -36, -36, -36, 9, -36, 60, -36, 24, -36, 73, -36, 38, 42,
			-13, -23, 13, 13, 23, 13, 13, 13, 13, 13, 24, 32, 13, 13, 38, 42, -23, -6, 31,
			-13, -13, 9, 41, -13, 46, -13, -13, -13, 47, -13, -13, -23, 31, 23, 2, 9, -23,
			-23, 46, -23, 32, -23, -23, 38, -23, -6, -6, 2, 9, 41, 60, 46, 24, -6, 73, 47,
			-6, 42, 31, 2, 31, 31, 60, 31, 31, 32, 73, 47, 38, 42, 2, 23, 23, 60, 46, 24,
			32, 73, 23, 23, 42, 9, 2, 2, 2, 24, 2, 73, 2, 2, 42, 41, 9, 9, 9, 32, 73, 47,
			38, 42, 60, 41, 41, 41, 41, 47, 41, 41, 60, 24, 60, 60, 60, 60, 42, 24, 32, 46,
			47, 38, 46, 24, 73, 47, 24, 42, 32, 32, 32, 42, 47, 38, 73, 47, 42, 42
	};
	test_coder_array(coder8, (int8_t*) din4, sizeof(din4)/sizeof(int8_t));
}

void test_elias_gamma() {
	test_log2();

	test_elias_gamma_single();
	test_elias_gamma_basic();
}

#endif /* ELIASGAMMA_HPP_ */
