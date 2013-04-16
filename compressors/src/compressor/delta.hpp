/**
 * delta.hpp
 * Delta encoder/decoder
 *
 * @author ishafer
 */

#ifndef DELTA_HPP_
#define DELTA_HPP_

#include <cassert>

#include "../util.hpp"

using namespace std;

template <typename T>
bool delta_enc_inplace(T* in, uint64_t inlen) {
	T tmp;
	T last = 0;
	for (uint64_t i = 0; i < inlen; ++i) {
		tmp = in[i];
		in[i] -= last;
		last = tmp;
	}
	return true;
}

template <typename T>
bool delta_dec_inplace(T* in, uint64_t inlen) {
	T last = 0;
	for (uint64_t i = 0; i < inlen; ++i) {
		in[i] += last;
		last = in[i];
	}
	return true;
}

void test_delta_basic() {
	int32_t din[] = {1, 2, 4, 5, 6, -3, 8};
	int32_t orig[sizeof(din)];
	memcpy(orig, din, sizeof(din));

	uint64_t inlen = sizeof(din)/sizeof(int32_t);
	assert( delta_enc_inplace<int32_t>(din, inlen) );

	int32_t exp[] = {1, 1, 2, 1, 1, -9, 11};
	assert( sizeof(din) == sizeof(exp) );

	assert( 0 == memcmp(din, exp, sizeof(din)) );

	assert( delta_dec_inplace(din, inlen) );

	assert( 0 == memcmp(din, orig, sizeof(din)) );
}

/**
 * XXX: the delta encoder/decoder relies on signed wraparound
 * for subtraction to work properly.
 * Here we ensure that we get our expected result.
 */
void test_delta_overflow() {
	int32_t din[] = {
			1,
			2,
			3,
			2147483647,
			-2147483647,
			4,
			5,
			-2147483647,
			2147483647
	};
	int32_t orig[sizeof(din)];
	memcpy(orig, din, sizeof(din));
	uint64_t inlen = sizeof(din)/sizeof(int32_t);

	assert( delta_enc_inplace<int32_t>(din, inlen) );
	assert( delta_dec_inplace<int32_t>(din, inlen) );

	assert( 0 == memcmp(din, orig, sizeof(din)) );

	int64_t din64[] = {
			1,
			2,
			3,
			9223372036854775807,
			-9223372036854775807,
			4,
			5,
			-9223372036854775807,
			9223372036854775807
	};
	int64_t orig64[sizeof(din64)];
	memcpy(orig64, din64, sizeof(din64));
	uint64_t inlen64 = sizeof(din64)/sizeof(int64_t);

	assert( delta_enc_inplace<int64_t>(din64, inlen64) );

	assert( delta_dec_inplace<int64_t>(din64, inlen64) );

	assert( 0 == memcmp(din64, orig64, sizeof(din)) );
}

#endif /* DELTA_HPP_ */
