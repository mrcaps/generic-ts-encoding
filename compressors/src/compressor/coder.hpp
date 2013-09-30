/**
 * coder.hpp
 * @brief Encoder/decoder interface
 * @author ishafer
 */

#ifndef CODER_HPP_
#define CODER_HPP_

const float BUF_SCALE_FACTOR = 2.0;

/**
 * @brief Interface to an encoder/decoder
 * vT: value type
 * bsT: bitstream backing type
 */
template<typename vT, typename bsT>
class Coder {
public:
	/**
	 * @brief encode values
	 * @param out place to dump output
	 * @param outsize size of output in bytes
	 * @param in input values
	 * @param insize input size in bytes
	 */
	virtual unsigned char* enc(
			unsigned char *out, uint64_t *outsize, vT *in, uint64_t insize) const = 0;
	/**
	 * @brief decode values
	 * @param out place to dump output
	 * @param outsize size of output in bytes
	 * @param in input values
	 * @param insize input size in bytes
	 * @returns new output pointer, if it has changed.
	 */
	virtual vT* dec(
			vT *out, uint64_t *outsize, unsigned char *in, uint64_t insize) const = 0;

	virtual ~Coder() {

	};
};

#define NBITS_WORD(t, x) (sizeof(t)*8 - __builtin_clz(x))

/**
 * Count the number of bits required to represent x
 */
template<typename T>
uint32_t nbits(T x) {
	if (sizeof(T) == sizeof(intptr_t)) {
		return (8*sizeof(intptr_t)) - __builtin_clz(x);
	} else {
		//do the obvious thing for now.
		uint32_t r = 0;
		while (x >>= 1) {
			++r;
		}
		return r + 1;
	}
}

/**
 * @brief Do a roundtrip encoding
 * @param coder the encoder/decoder to use
 * @param arr the array to encode/decode
 * @param npoints number of points in the array
 */
template<typename vT, typename bsT>
void test_coder_array(Coder<vT, bsT> &coder, vT *arr, uint64_t npoints) {
	unsigned char *outbits = static_cast<unsigned char*>(
			calloc(npoints*BUF_SCALE_FACTOR+12, sizeof(vT)));
	uint64_t outsize = sizeof(vT)*(npoints*BUF_SCALE_FACTOR+12);
	uint64_t insize = npoints;
	outbits = coder.enc(outbits, &outsize, static_cast<vT*>(arr), insize);
	vT* dout = static_cast<vT*>(malloc(sizeof(vT)*(npoints*BUF_SCALE_FACTOR+12)));
	dout = coder.dec(dout, &insize, outbits, outsize);
	if ( memcmp(dout, arr, npoints*sizeof(vT)) ) {
		cout << "Roundtrip error!" << endl;
		cout << " in:";
		print_arr(arr, min(static_cast<uint64_t>(20), npoints));
		cout << endl;
		cout << "out:";
		print_arr(dout, min(static_cast<uint64_t>(20), npoints));
	}

	free(dout);
	free(outbits);
}

#endif /* CODER_HPP_ */
