/**
 * Main
 * @author ishafer
 */
#include <iostream>
#include <cstring>
#include <string>

#include "metastore.hpp"
#include "compressor.hpp"

using namespace std;

void test_all() {
	test_metastore();
	test_compressor();
}

void print_result(bool deltaenc, CoderName name, vstream_res &res) {
	cout << res.p.vname << "," <<
			res.p.tpath << "," <<
			res.p.vpath << "," <<
			res.p.vsize << ",";
	test_roundtrip(deltaenc, name, res.p);
}

void run(bool deltaenc, int32_t limit) {
	MetaStore ms("G:/tmp/combined.db", Config::get("dataloc"));
	ms.open();
	ms.start_iter();

	int sdx = 0;
	for (vstream_res res; (res = ms.next_row()).success; ) {
		print_result(deltaenc, ELIAS_GAMMA, res);
		print_result(deltaenc, ELIAS_DELTA, res);
		print_result(deltaenc, LOG_HUFFMAN, res);
		print_result(deltaenc, ZLIB, res);

		if (++sdx > limit) {
			cout << "STOPPED" << endl;
			break;
		}
	}

	ms.close();
}

void usage(char* argv[]) {
	cout << "Usage: " << argv[0] << " [fn] [args]" << endl;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		usage(argv);
		return 1;
	}

	string fn(argv[1]);

	if (fn == "test") {
		test_all();
	} else if (fn == "runall") {
		run(true, numeric_limits<int32_t>::max());
	} else if (fn == "runsome") {
		run(true, 100);
	}

	return 0;
}
