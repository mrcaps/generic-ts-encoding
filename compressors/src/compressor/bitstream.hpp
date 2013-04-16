/**
 * bitstream.hpp
 * @author ishafer
 */

#ifndef BITSTREAM_HPP_
#define BITSTREAM_HPP_

#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <bitset>
//#include <type_traits>

using namespace std;

enum mode {READ, WRITE};

/**
 * @brief simple bitstream.
 * Parameterized by backing array type and read/write type
 * portions are from one I wrote ~8 years ago
 * backing and rwsize must be unsigned
 */
template <typename backing, typename rwsize>
class BitStream {
private:
	//does this BitStream own the backing?
	bool vals_owned;
	backing *vals;
	uint64_t nvals;
	backing *cur;
	backing *end;

	//current write mask
	uint8_t mask;
	uint8_t backsize;

	mode status;

	//last mask written
	uint8_t last_mask;
	//last backing position
	backing *last_cur;

public:
	BitStream(uint64_t nvals) {
		vals = (backing*) calloc(nvals, sizeof(backing));
		this->nvals = nvals;
		cur = vals;
		end = vals + nvals;
		vals_owned = true;

		init_write();
	}

	/**
	 * @brief Create a bitstream for writing backed by the given array.
	 * NOTE: backing* may become invalid if the bitstream for a write
	 * stream if the bitstream needs to allocate extra space
	 * @param openmode should we open for read or write?
	 */
	BitStream(backing* back, uint64_t nvals, mode openmode) {
		vals = back;
		this->nvals = nvals;
		cur = vals;
		end = vals + nvals;
		vals_owned = false;

		if (openmode == READ) {
			init_read();
		} else if (openmode == WRITE) {
			init_write();
		}
	}

	void init_read() {
		backsize = sizeof(backing)*8;
		mask = backsize;

		status = READ;
		last_mask = mask;
		last_cur = vals + nvals;
	}

	void init_write() {
		backsize = sizeof(backing)*8;
		mask = backsize;

		status = WRITE;
		last_mask = mask;
		last_cur = cur;
	}

	~BitStream() {
		if (vals_owned) {
			free(vals);
		}
	}

	/**
	 * @brief rewind bit stream and open for read
	 */
	void rewind() {
		if (status == WRITE) {
			last_mask = mask;
			last_cur = cur;
			status = READ;
		}

		cur = vals;
		mask = backsize;
	}

	/**
	 * @brief write multiple bits to the stream
	 * @param bits the bits to write
	 * @param nbits the number of bits in bits
	 */
	void write_bits(rwsize bits, int32_t nbits) {
		if (0 == nbits) {
			return;
		}
		//if we're about to write more than remains, expansion needed.
		if (cur + nbits / backsize + 1 > end) {
			expand_backing();
		}

		//position in bits
		int32_t pos = 0;

		//bits of unaligned
		while (mask > 0 && pos != nbits - 1) {
			//cout << "write begin bit" << endl;
			//cout << "begin bit " << ((bits & ((backing) 1 << (nbits - pos - 1))) > 0) << endl;
			write_bit( (bits & (static_cast<rwsize>(1) << (nbits - pos - 1))) > 0 );
			++pos;
		}

		//blocks of middle bytes
		for (; pos < nbits - backsize + 1; pos += backsize) {
			++cur;

			//cout << "pos=" << pos
			//		<< " cur=" << (uint64_t) cur
			//		<< " nbits=" << nbits
			//		<< " backsize=" << (uint32_t) backsize
			//		<< " nb-p-b=" << (nbits-pos) << endl;
			//cout << "mid bits " << bitset<64>(bits >> (nbits - pos - backsize)) << endl;

			*cur = (*cur) | (bits >> (nbits - pos - backsize));
			//print_backing();
			//cout << endl;
		}

		//bits after blocks
		while (pos < nbits) {
			write_bit(  (bits & (static_cast<rwsize>(1) << (nbits - pos - 1))) > 0 );
			++pos;
		}
	}

	void expand_backing() {
		//expand bitstream by a factor of 1.5
		// (ArrayList style)
		//cout << "expand back from " << nvals << endl;
		uint64_t newvals = nvals * 1.5 + sizeof(rwsize)/sizeof(backing);

		//cout << "before:" << endl;
		//print_backing(); cout << endl;
		assert (cur <= end);
		backing* res = (backing*) realloc(vals, sizeof(backing)*newvals);

		memset(res + nvals, 0, newvals - nvals);
		if (NULL == res) {
			cerr << "failed to expand bitstream" << endl;
		} else {
			cur = (res - vals) + cur;
			vals = res;
			nvals = newvals;
			end = vals + newvals;
		}

		//cout << "after:" << endl;
		//print_backing(); cout << endl;
	}

	/**
	 * @brief write a single bit to the stream
	 */
	void write_bit(bool bit) {
		//cout << "write bit" << (bit ? "1" : "0") << endl;
		if (0 == mask) {
			++cur;
			mask = sizeof(backing)*8;
			if (cur >= end - 1) {
				expand_backing();
			}
		}

		*cur = *cur | bit << (--mask);
	}

	bool read_bit() {
		if (status == WRITE) {
			cerr << "bitstream::read_bit cannot be used in write mode" << endl;
			return false;
		}

		//end of a byte
		if (mask == 0) {
			if (cur < last_cur) {
				++cur;
				mask = sizeof(backing)*8;
			} else {
				cerr << "bitstream::read_bit going past end of bitstream" << endl;
			}
		} else if ( (cur == last_cur) && (mask < last_mask) ) {
			cerr << "bitstream::read_bit going past end of bitstream" << endl;
		}

		return (*(cur) & 1 << (--mask)) > 0;
	}

	rwsize read_bits(int32_t nbits) {
		if (nbits == 0) {
			return 0;
		}

		int32_t pos = 0;
		rwsize bits = 0;

		//bits before bytes
		while (mask > 0 && pos != nbits - 1) {
			rwsize bit = read_bit();
			//cout << "read begin bit@" << pos << "->" << (nbits - pos - 1) << " " << bit << endl;
			bits = bits | static_cast<rwsize>(bit << (nbits - pos - 1));
			++pos;
		}

		//cout << "after begin bits was " (uint64_t) bits << endl;

		//blocks of middle bytes
		for (; pos < nbits - backsize + 1; pos += backsize) {
			++cur;
			//cout << "*cur=" << bitset<8>(*cur) << " -> " << (nbits - pos - backsize) << endl;
			bits = bits | ((static_cast<rwsize>(*cur)) << (nbits - pos - backsize));
			//cout << "after block was " << (uint64_t) bits << endl;
		}

		//cout << "after mid bits was" << bits << endl;

		//bits after bytes
		while (pos < nbits) {
			//cout << "end bit@" << pos << " ->" << (nbits - pos - 1) << endl;
			bits = bits | (read_bit() << (nbits - pos - 1));
			++pos;
		}

		return bits;
	}

	/**
	 * Check if the bitstream is ready when reading
	 */
	bool ready() {
		return (cur < last_cur || ( (cur == last_cur) && (mask > last_mask) ) );
	}

	/**
	 * @returns total number of backing sizes written
	 */
	uint32_t written_size() {
		if (status == READ) {
			return last_cur - vals + 1;
		} else if (status == WRITE) {
			return cur - vals + 1;
		}
		cerr << "Unknown status:" << status << endl;
		return 0;
	}

	/**
	 * @returns total number of bits written
	 */
	uint64_t written_bits() {
		return written_size()*sizeof(backing)*8 - mask;
	}

	void print_state() {
		cout << "{";
		cout << "vals=" << (void*) vals;
		cout << " cur=" << (void*) cur;
		cout << " end=" << (void*) end;
		cout << " last_cur=" << (void*) last_cur;

		cout << " backsize=" << (uint32_t) backsize;
		cout << " mask=" << (uint32_t) mask;
		cout << " last_mask=" << (uint32_t) last_mask;

		cout << "}" << endl;
	}

	void print_bits() {
		rewind();
		cout << "[";
		while (ready()) {
			cout << (read_bit() ? "1" : "0");
		}
		cout << "]";
	}

	void print_backing() {
		cout << "[";
		cout << "mask=" << (uint32_t) mask;
		for (backing *ptr = vals; ptr < end && ptr < vals + 40; ++ptr) {
			cout << ((ptr == cur) ? ">" : " ") << bitset<sizeof(backing)*8>(*ptr);
		}
		cout << "]";
	}

	backing* get_backing() {
		return vals;
	}
};

template<typename backing, typename rwsize>
static void assert_compare(BitStream<backing, rwsize> &bs, bool* vals, uint32_t len) {
	bs.rewind();
	for (uint32_t i = 0; i < len; ++i) {
		assert( bs.ready() );
		bool bit = bs.read_bit();
		if( bit != vals[i] ) {
			cout << "bitstrings not equal at position " << i << " (expected " << vals[i] << " got " << bit << ")" << endl;
		}
	}
	assert( !bs.ready() );
}

static void test_bitstream_basic() {
	BitStream<uint8_t, uint8_t> bs(10);
	bs.write_bit(1);
	bs.write_bit(0);
	bs.write_bit(1);

	bs.rewind();

	assert(bs.ready());
	assert(bs.read_bit());

	assert(bs.ready());
	assert(!bs.read_bit());

	assert(bs.ready());
	assert(bs.read_bit());

	assert(!bs.ready());
}

static void test_bitstream_multibit() {
	BitStream<uint8_t, uint8_t> bs1(8);
	bs1.write_bit(1);
	bs1.write_bit(0);
	bs1.write_bits(0b1110101, 7);
	bool cmp1[] = {1,0, 1,1,1,0,1,0,1};
	assert_compare(bs1, cmp1, 9);

	BitStream<uint8_t, uint8_t> bs2(8);
	bs2.write_bits(0b110, 3);
	bs2.write_bit(1);
	bs2.write_bits(0b1110, 4);
	bs2.write_bits(0b101, 3);
	bool cmp2[] = {1,1,0, 1, 1,1,1,0, 1,0,1};
	assert_compare(bs2, cmp2, 11);

	BitStream<char, uint32_t> bs3(8);
	bs3.write_bits(0b10, 2);
	bs3.write_bits(0b11, 2);
	bs3.write_bit(1);
	bs3.write_bits(0b1110, 4);
	bs3.write_bits(0b101, 3);
	bool cmp3[] = {1,0, 1,1, 1, 1,1,1,0, 1,0,1};
	assert_compare(bs3, cmp3, 12);

	BitStream<unsigned char, uint32_t> bs4(10);
	bs4.write_bits(0, 31);
	bs4.write_bits(3278920000u, 32);
	bs4.rewind();

	assert( 0 == bs4.read_bits(31) );
	assert( 3278920000u == bs4.read_bits(32) );
}

static void test_bitstream_expand() {
	BitStream<uint8_t, uint8_t> bs1(2);
	for (int i = 1; i < 29; ++i) {
		bs1.write_bit(i % 2);
	}

	bool cmp1[] = {
		1,0, 1,0, 1,0, 1,0, 1,0, 1,0, 1,0,
		1,0, 1,0, 1,0, 1,0, 1,0, 1,0, 1,0
	};
	assert_compare(bs1, cmp1, 28);

	BitStream<uint8_t, uint8_t> bs2(2);
	for (int i = 0; i < 12; ++i) {
		bs2.write_bits(0b11011, 5);
	}
	bool cmp2[] = {
		1,1,0,1,1, 1,1,0,1,1, 1,1,0,1,1, 1,1,0,1,1,
		1,1,0,1,1, 1,1,0,1,1, 1,1,0,1,1, 1,1,0,1,1,
		1,1,0,1,1, 1,1,0,1,1, 1,1,0,1,1, 1,1,0,1,1
	};
	assert_compare(bs2, cmp2, 12*5);
}

void test_bitstream_readmany() {
	BitStream<uint8_t, uint32_t> bs1(2);
	for (int i = 1; i < 29; ++i) {
		bs1.write_bit(i % 2);
	}
	bs1.rewind();

	assert( bs1.read_bits(4) == 0b1010 );
	assert( bs1.read_bits(2) == 0b10 );
	assert( bs1.read_bits(6) == 0b101010 );
	assert( bs1.read_bits(8) == 0b10101010 ); //->20 bits
	assert( bs1.read_bits(3) == 0b101 );
	assert( bs1.read_bits(3) == 0b010 ); //->26 bits
	assert( bs1.read_bits(2) == 0b10 );
	assert( !bs1.ready() );
}

void test_bitstream_64bit() {
	BitStream<uint8_t, int64_t> bs1(2);
	bs1.write_bit(1);
	bs1.write_bits(0b111111110000000011111111, 24);
	bs1.write_bits(0b10101010101010101010101010, 26);

	bool cmp1[] = {
		1,
		1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
		1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0
	};
	assert_compare(bs1, cmp1, 1+24+26);

	BitStream<unsigned char, uint64_t> bs2(10);
	bs2.write_bits(0b111001110001001111101100000001000000, 36);
	bool cmp2[] = {
		1,1,1,0,0,1,1,1,0,0,0,1,0,0,1,1,1,1,1,0,1,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0
	};
	assert_compare(bs2, cmp2, 36);
}

void test_bitstream() {
	test_bitstream_basic();
	test_bitstream_multibit();
	test_bitstream_expand();
	test_bitstream_readmany();
	test_bitstream_64bit();
}

#endif /* BITSTREAM_HPP_ */
