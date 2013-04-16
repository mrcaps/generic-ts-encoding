/*
 * eliasomega.hpp
 * @brief Elias omega encoder/decoder
 * @author ishafer
 */

#ifndef ELIASDELTA_HPP_
#define ELIASDELTA_HPP_

#include <iostream>

#include "coder.hpp"
#include "bitstream.hpp"
#include "zigzag.hpp"
#include "../util.hpp"

using namespace std;

/**
 * Elias-gamma encoding with zig-zag first.
 */
template<typename vT, typename bsT>
class EliasDelta : public Coder<vT, bsT> {
public:
	EliasDelta() {};
	~EliasDelta() {};

	unsigned char* enc(
			unsigned char *out,
			uint64_t *outsize,
			vT *in,
			uint64_t insize) const {
		BitStream<unsigned char, bsT> bs(out, *outsize, WRITE);
		for (uint64_t i = 0; i < insize; ++i) {
			uint64_t v = ZIGZAG_ENC(static_cast<int64_t>(in[i])) + 1;
			uint32_t nb = nbits(v);
			uint32_t nb_nb = nbits(nb);
			//cout << "in[" << i << "]=" << in[i] << "->" << v <<
			//	" (nb=" << nb << " nb_nb=" << nb_nb << ")" << endl;
			bs.write_bits(0, nb_nb-1);
			bs.write_bits(nb, nb_nb);
			bs.write_bits(v & ~(static_cast<uint64_t>(1) << nb), nb-1);
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
			uint32_t nb_nb = 0;
			while (bs.ready() && (!bs.read_bit())) {
				++nb_nb;
			}
			uint64_t nb = (bs.read_bits(nb_nb) | (static_cast<bsT>(1) << nb_nb));
			uint64_t v = (bs.read_bits(nb - 1) | (static_cast<bsT>(1) << (nb-1)));
			//cout << "out[" << i << "]=" << v <<
			//		" (nb=" << nb << " nb_nb=" << nb_nb << ")" << endl;
			out[i] = ZIGZAG_DEC(v - 1);
		}
		return out;
	}

private:
	DISALLOW_EVIL_CONSTRUCTORS(EliasDelta);
};

void test_elias_delta_single() {
	EliasDelta<int32_t, uint32_t> coder32;

	int32_t din1[] = {0,-1,1,-2,2,-3,3};
	unsigned char out[20];
	memset(&out, 0, sizeof(out));
	uint64_t outsize = sizeof(out);
	uint64_t insize = sizeof(din1)/sizeof(int32_t);
	coder32.enc(static_cast<unsigned char*>(out),
			&outsize, static_cast<int32_t*>(din1), insize);

	int32_t* dout = (int32_t*) malloc(sizeof(din1));
	coder32.dec(dout, &insize, out, insize);

	assert( 0 == memcmp(dout, din1, sizeof(din1)) );
}

void test_elias_delta_basic() {
	EliasDelta<int32_t, uint32_t> coder32;
	EliasDelta<int64_t, uint64_t> coder64;

	int32_t din[] = {1, 2, 4, 5, 6, -3, 8};
	test_coder_array(coder32, (int32_t*) din, sizeof(din)/sizeof(int32_t));

	int32_t din2[] = {0, 181817, 363636, 545454, 363636, 363636, 545454, 1, 2, 3, 4, 5};
	test_coder_array(coder32, (int32_t*) din2, sizeof(din2)/sizeof(int32_t));

	int64_t din3[] = {31014740000, 31000620000, 30985390000, 30968450000, 30950330000};
	test_coder_array(coder64, (int64_t*) din3, sizeof(din3)/sizeof(int64_t));

	int32_t din4[] = {
			1639460000, 1640110000, 1641680000, 1646900000, 1647220000,
			1648980000, 1650290000, 1652380000, 1653190000, 1653260000,
			1652190000, 1651490000, 1649530000, 1647570000, 1644160000,
			1642830000, 1641360000, 1641000000, 1642480000, 1643660000,
			1648450000, 1657720000, 1666430000, 1677100000, 1683800000,
			1692180000, 1696240000, 1705290000, 1712810000, 1714260000,
			1710440000, 1705460000, 1699070000, 1696200000, 1689210000,
			1685700000, 1666890000, 1657130000, 1645400000, 1643200000,
			1637260000, 1631690000, 1628840000, 1621830000, 1620370000,
			1621360000, 1622070000, 1624870000, 1631410000, 1634910000,
			1638800000, 1642160000, 1647080000, 1650320000, 1652030000,
			1655410000, 1656110000, 1655970000, 1655240000, 1653450000,
			1652610000, 1651430000, 1649960000, 1649350000, 1648730000,
			1648380000, 1648240000, 1648470000, 1649040000, 1649970000,
			1650780000, 1652010000, 1653720000, 1655910000, 1656770000,
			1661810000, 1663380000, 1666320000, 1667650000, 1670830000,
			1673500000, 1670400000, 1654850000, 1638050000, 1619900000,
			1612350000, 1593450000, 1569450000, 1543010000, 1512760000,
			1489670000, 1470650000, 1423840000, 1335710000, 1265810000,
			1227750000, 1204340000, 1194490000, 1063900000, 1003400000,
			924231000, 858287000, 834796000, 839589000, 818629000, 799141000,
			439253000, 274616000, 211065000, 151484000, 89913900, 63997000,
			-539350, -51248200, -71352600, -82286400, -77967500, -74371500,
			-84385900, -81122200, -72204000, -62473300, -54703200, -49908000,
			-45730100, -42154200, -39724800, -37987500, -37041000, -34410800,
			-33532200, -31991500, -30047800, -28381800, -26861500, -25037500,
			-24200800, -20969500, -19778100, -17869500, -16235000, -14782600,
			-13130800, -11461000, -10483800, -9310610, -7740780, -6313280,
			-5484370, -4291490, -3629660, -2969250, -1840610, -320088, 595526,
			1194470, 1923490, 2827700, 3766750, 4238710, 4571410, 4578070,
			4578200, 4578170, 4578170, 4578170, 4578170, 4578170, 4578170,
			4578170, 4578170, 4578170, 4578170, 4578170, 4578170, 4578170,
			4578170, 4578170, 4578170, 4578170, 4578170, 4578170, 4578170,
			4578170, 4578170
	};
	test_coder_array(coder32, (int32_t*) din4, sizeof(din4)/sizeof(int32_t));
}

void test_elias_delta() {
	test_elias_delta_single();
	test_elias_delta_basic();
}

#endif /* ELIASDELTA_HPP_ */
