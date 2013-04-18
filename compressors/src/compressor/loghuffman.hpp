/*
 * loghuffman.hpp
 * @brief log-huffman encoder/decoder
 * @author ishafer
 */

#ifndef LOGHUFFMAN_HPP_
#define LOGHUFFMAN_HPP_

#include "coder.hpp"
#include "bitstream.hpp"
#include "zigzag.hpp"
#include "../util.hpp"

#include <queue>
#include <limits>
#include <bitset>

typedef map<int8_t, uint64_t> huff_hist;
void print_huff_hist(huff_hist hist) {
	for (huff_hist::iterator it = hist.begin(); it != hist.end(); ++it) {
		cout << "hist k=" << (*it).first << " v=" << (*it).second << endl;
	}
}

/**
 * Bounds on the keys in the given histogram
 */
pair<int32_t, int32_t> bounds(huff_hist hist) {
	int8_t minv = numeric_limits<int8_t>::max();
	int8_t maxv = numeric_limits<int8_t>::min();
	bool ok = false;
	for (huff_hist::iterator it = hist.begin(); it != hist.end(); ++it) {
		minv = min(minv, it->first);
		maxv = max(maxv, it->first);
		ok = true;
	}
	if (!ok) {
		cerr << "No bounds for an empty huff_hist!" << endl;
	}
	return pair<int8_t, int8_t>(minv, maxv);
}

struct huffnode {
	static const int32_t INTERNAL = -1;
	static const uint64_t NOCOUNT;
public:
	int32_t val; //number of bits; -1 for internal node
	uint64_t count;
	huffnode *left;
	huffnode *right;

	huffnode(int32_t myval, uint64_t mycount) :
		val(myval), count(mycount) {
		left = NULL;
		right = NULL;
	}

	bool operator<(const huffnode& other) const {
		return count < other.count;
	}

	//friend ostream& operator<<(ostream &out, const huffnode &node);
};
const uint64_t huffnode::NOCOUNT = numeric_limits<uint64_t>::max();
ostream& operator<<(ostream &out, const huffnode &node) {
	return out << "[node val=" << node.val << " count=" << node.count << "]";
}

//TODO: clean up tree
class huffnode_compare {
public:
	bool operator() (const huffnode* lhs, const huffnode* rhs) {
		return !((*lhs) < (*rhs));
	}
};

void print_tree(huffnode *node, int tablvl) {
	for (int i = 0; i < tablvl; ++i) {
		cout << "| ";
	}
	cout << "node val=" << node->val << " count=" << node->count << endl;
	if (NULL != node->left) {
		print_tree(node->left, tablvl + 1);
	}
	if (NULL != node->right) {
		print_tree(node->right, tablvl + 1);
	}
}

/**
 * Check for value-equality of trees
 */
bool trees_equal(huffnode *t1, huffnode *t2) {
	if ((t1->left == NULL) ^ (t2->left == NULL)) {
		return false;
	} else if (t1->left != NULL) {
		if (!trees_equal(t1->left, t2->left)) {
			return false;
		}
	}

	if ((t1->right == NULL) ^ (t2->right == NULL)) {
		return false;
	} else if (t1->right != NULL) {
		if (!trees_equal(t1->right, t2->right)) {
			return false;
		}
	}

	return (t1->val == t2->val);
}

huffnode* build_tree(huff_hist &themap) {
	//push on to priority queue
	priority_queue<huffnode*, vector<huffnode*>, huffnode_compare > queue;
	for (huff_hist::iterator it = themap.begin(); it != themap.end(); ++it) {
		queue.push(new huffnode((*it).first, (*it).second));
	}

	//dump queue
	/*
	while (!queue.empty()) {
		cout << "top=" << (*queue.top()) << endl;
		queue.pop();
	}*/

	//build tree
	while (queue.size() > 1) {
		huffnode *h1 = queue.top();
		queue.pop();
		huffnode *h2 = queue.top();
		queue.pop();

		huffnode *mid = new huffnode(huffnode::INTERNAL, h1->count + h2->count);
		mid->left = h1;
		mid->right = h2;

		queue.push(mid);
	}

	return queue.top();
}

/**
 * @param bs bitstream to write tree to
 * @param node the node to write
 * @param valbits the number of value bits
 * @param valmin subtract this minimum value
 */
template<typename T>
void tree_to_bitstream_inner(
		BitStream<unsigned char, T> &bs,
		huffnode* node,
		int32_t valbits,
		uint32_t valmin) {
	if (NULL == node) {
		return;
	}
	if (node->val == huffnode::INTERNAL) {
		bs.write_bit(0);
		tree_to_bitstream_inner(bs, node->left, valbits, valmin);
		tree_to_bitstream_inner(bs, node->right, valbits, valmin);
	} else {
		bs.write_bit(1);
		bs.write_bits(node->val - valmin, valbits);
	}
}

template<typename T>
huffnode* bitstream_to_tree(BitStream<unsigned char, T> &bs) {
	int32_t minval = bs.read_bits(7);
	int32_t valbits = bs.read_bits(7);
	return tree_to_bitstream_inner(bs, minval, valbits);
}

template<typename T>
huffnode* tree_to_bitstream_inner(
		BitStream<unsigned char, T> &bs,
		int32_t minval,
		int32_t valbits) {
	bool bit = bs.read_bit();
	if (!bit) {
		huffnode *node = new huffnode(huffnode::INTERNAL, huffnode::NOCOUNT);
		node->left = tree_to_bitstream_inner(bs, minval, valbits);
		node->right = tree_to_bitstream_inner(bs, minval, valbits);
		return node;
	} else {
		return new huffnode(bs.read_bits(valbits) + minval, huffnode::NOCOUNT);
	}
}

/**
 * Write a huffman tree to bits
 * @param bs bitstream to write the tree to
 * @param tree root of the tree to write
 * @param bounds (as from #bounds())
 */
template<typename T>
void tree_to_bitstream(
		BitStream<unsigned char, T> &bs,
		huffnode* tree,
		pair<int8_t, int8_t> bounds) {
	//first, send the minimum alphabet value
	// (in 7 bits)
	bs.write_bits(static_cast<uint32_t>(bounds.first), 7);
	//then, the number of bits required to
	// encode difference from the min bound.
	uint32_t valbits = static_cast<uint32_t>(nbits(bounds.second - bounds.first));
	//cout << "bounds difference=" << bounds.second - bounds.first << " -> nbits="
	//		<< nbits(bounds.second-bounds.first) << endl;
	bs.write_bits(valbits, 7);

	//now write the tree
	tree_to_bitstream_inner(bs, tree, valbits, bounds.first);
}

/**
 * Lookup entries are pairs of (bits, nbits)
 */
typedef pair<uint32_t, int32_t> lookup_entry;

/**
 * Reverse bits in a byte
 * Due to http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith32Bits
 */
uint8_t reverse_bits_8(uint8_t b) {
	return ((b * 0x0802LU & 0x22110LU) | (b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

/**
 * Reverse bits in a word
 * Due to http://graphics.stanford.edu/~seander/bithacks.html#BitReverseObvious
 */
uint64_t reverse_bits_64(uint64_t v) {
	uint64_t r = v; // r will be reversed bits of v; first get LSB of v
	int s = sizeof(v)*8 - 1; // extra shift needed at end

	for (v >>= 1; v; v >>= 1)
	{
		r <<= 1;
		r |= v & 1;
		s--;
	}
	r <<= s; // shift when v's highest bits are zero
	return r;
}

/**
 * Lookup vector for encoding
 * @param vec (out param) vector to fill with LUT entries
 * @param node current recursion point
 * @param bits_so_far bit string to this pont in the tree
 * @param depth current depth in the tree
 */
void tree_to_lookup_inner(
		vector<lookup_entry> &vec,
		huffnode *node,
		uint64_t bits_so_far,
		int32_t depth) {
	if (NULL == node) {
		assert(false);
		return;
	} else if (node->val != huffnode::INTERNAL) {
		vec[node->val] = lookup_entry(
				reverse_bits_64(bits_so_far) >> (sizeof(bits_so_far)*8-depth), depth);
	} else {
		tree_to_lookup_inner(vec, node->left, bits_so_far, depth+1);
		tree_to_lookup_inner(vec, node->right, bits_so_far | 1 << depth, depth+1);
	}
}

/**
 * @param node huffman node to write
 * @returns pairs of <code, nbits>
 */
vector<lookup_entry> tree_to_lookup(huffnode *node) {
	vector<lookup_entry> entries(64);
	tree_to_lookup_inner(entries, node, 0, 0);
	return entries;
}

template<typename T>
int32_t next_huffcode(
		BitStream<unsigned char, T> &bs,
		huffnode* tree) {
	while (tree->val == huffnode::INTERNAL) {
		tree = bs.read_bit() ? tree->right : tree->left;
	}
	return tree->val;
}


/**
 * Huffman coding on logs of values followed by binary values
 * May leave some garbage bits at the end of the output.
 */
template<typename vT, typename bsT>
class LogHuffman : public Coder<vT, bsT> {
public:
	LogHuffman() {};
	~LogHuffman() {};

	unsigned char* enc(
			unsigned char *out,
			uint64_t *outsize,
			vT *in,
			uint64_t insize) const {
		BitStream<unsigned char, bsT> bs(out, *outsize, WRITE);
		//build probability distribution
		huff_hist hist;
		for (uint64_t i = 0; i < insize; ++i) {
			int32_t nb = nbits(ZIGZAG_ENC(static_cast<int64_t>(in[i])) + 1);
			if (hist.find(nb) == hist.end()) {
				hist[nb] = 0;
			}
			hist[nb] += 1;
		}

		huffnode *tree = build_tree(hist);
		//send the tree
		tree_to_bitstream(bs, tree, bounds(hist));
		vector<lookup_entry> lut = tree_to_lookup(tree);

		//cout << "ENCODE tree:" << endl;
		//print_tree(tree, 0);
		//cout << "dictionary bits: " << bs.written_bits() << endl;

		for (uint64_t i = 0; i < insize; ++i) {
			uint64_t v = ZIGZAG_ENC(static_cast<int64_t>(in[i])) + 1;
			uint32_t nb = nbits(v);
			//write huffman code for nbits
			bs.write_bits(lut[nb].first, lut[nb].second);
			//write the value
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

		//inflate the dictionary
		huffnode *tree = bitstream_to_tree(bs);

		//cout << "DECODE tree:" << endl;
		//print_tree(tree, 0);
		//cout << "bitstream is now:" << endl;
		//bs.print_backing();
		//cout << flush << endl;

		for (uint64_t i = 0; i < *outsize; ++i) {
			int32_t nb = next_huffcode(bs, tree);
			uint64_t v = bs.read_bits(nb-1) | (static_cast<uint64_t>(1) << (nb-1));
			out[i] = ZIGZAG_DEC(v - 1);
		}
		return out;
	}

private:
	DISALLOW_EVIL_CONSTRUCTORS(LogHuffman);
};

/**
 * Histogram entry
 */
void test_tree() {
	huff_hist hist;
	hist[0] = 1;
	hist[1] = 1;
	hist[2] = 2;
	hist[3] = 4;
	hist[4] = 3;
	hist[5] = 2;

	huffnode* root = build_tree(hist);
	//order -> 2, 8, 5

	BitStream<unsigned char, int32_t> bs(8);
	tree_to_bitstream(bs, root, bounds(hist));

	bs.rewind();
	huffnode* tree = bitstream_to_tree(bs);

	assert( trees_equal(root, tree) );

	//print_tree(tree, 0);

	vector<lookup_entry> lut = tree_to_lookup(tree);

	//to print the lookup table
	int32_t lutdx = 0;
	for (vector<lookup_entry>::iterator it = lut.begin(); it != lut.end(); ++it) {
		//cout << "lut i=" <<
		//		lutdx << " k=" <<
		//		bitset<8>((*it).first) << " v=" << (*it).second << endl;
		uint32_t bits = (*it).first;
		int32_t nbits = (*it).second;
		switch (lutdx) {
		case 0:
			assert( bits == 0b1101 );
			assert( nbits == 4 );
			break;
		case 1:
			assert( bits == 0b1100 );
			assert( nbits == 4 );
			break;
		case 2:
			assert( bits == 0b0111 );
			assert( nbits == 3 );
			break;
		case 3:
			assert( bits == 0b10 );
			assert( nbits == 2 );
			break;
		case 4:
			assert( bits == 0b1 );
			assert( nbits == 2 );
			break;
		case 5:
			assert( bits == 0b0 );
			assert( nbits == 2 );
			break;
		}
		++lutdx;
	}
}

void test_loghuffman_basic() {
	LogHuffman<int32_t, uint32_t> coder32;
	LogHuffman<int64_t, uint64_t> coder64;

	int32_t din[] = {1, 2, 4, 5, 6, -3, 8};
	test_coder_array(coder32, (int32_t*) din, sizeof(din)/sizeof(int32_t));

	int32_t din2[] = {0, 181817, 363636, 545454, 363636, 363636, 545454, 1, 2, 3, 4, 5};
	test_coder_array(coder32, (int32_t*) din2, sizeof(din2)/sizeof(int32_t));

	int64_t din3[] = {31014740000, 31000620000, 30985390000, 30968450000, 30950330000};
	test_coder_array(coder64, (int64_t*) din3, sizeof(din3)/sizeof(int64_t));
}

void test_loghuffman() {
	test_tree();
	test_loghuffman_basic();
}

#endif /* LOGHUFFMAN_HPP_ */
