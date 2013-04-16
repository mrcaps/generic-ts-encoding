/**
 * zigzag.hpp
 * @brief ZigZag integer signed->unsigned conversion
 *
 * Magical bit operations thanks to Protocol Buffers and
 * 	http://stackoverflow.com/questions/2210923/zig-zag-decoding
 *
 * @author ishafer
 */

#ifndef ZIGZAG_HPP_
#define ZIGZAG_HPP_

#include <cassert>

#define ZIGZAG_ENC(x) (((x) << 1) ^ ((x) >> (8*sizeof(x)-1)))
#define ZIGZAG_DEC(x) (((x) >> 1) ^ -((x) & 1))

template <typename T>
bool zigzag_enc_inplace(T* in, uint64_t inlen) {
	for (uint64_t i = 0; i < inlen; ++i) {
		in[i] = ZIGZAG_ENC(in[i]);
	}
	return true;
}

template <typename T>
bool zigzag_dec_inplace(T* in, uint64_t inlen) {
	for (uint64_t i = 0; i < inlen; ++i) {
		in[i] = ZIGZAG_DEC(in[i]);
	}
	return true;
}

void test_zigzag() {
	int64_t din[] = {-6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6,
			2147483647, -2147483647, 31014740000, 1639460000};
	int64_t orig[sizeof(din)];
	memcpy(orig, din, sizeof(din));
	uint64_t inlen = sizeof(din)/sizeof(int32_t);

	assert( zigzag_enc_inplace<int64_t>(din, inlen) );

	uint64_t exp[] = {11, 9, 7, 5, 3, 1, 0, 2, 4, 6, 8, 10, 12,
			4294967294u, 4294967293u, 62029480000u, 3278920000u};
	assert( sizeof(din) == sizeof(exp) );
	assert( 0 == memcmp((uint64_t*) din, exp, sizeof(din)) );

	assert( zigzag_dec_inplace<uint64_t>((uint64_t*) din, inlen) );

	assert( 0 == memcmp(din, orig, sizeof(din)) );

	assert( (uint64_t) ZIGZAG_ENC((int64_t) 1639460000) == 3278920000u );
}


#endif /* ZIGZAG_HPP_ */
