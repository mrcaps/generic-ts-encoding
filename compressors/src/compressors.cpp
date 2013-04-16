/**
 * Interface for compressors
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

void print_result(CoderName name, vstream_res &res) {
	cout << res.p.vname << "," <<
			res.p.tpath << "," <<
			res.p.vpath << "," <<
			res.p.vsize << ",";
	test_roundtrip(name, res.p);
}

void run(int32_t limit) {
	MetaStore ms("G:/tmp/combined.db", Config::get("dataloc"));
	ms.open();
	ms.start_iter();

	int sdx = 0;
	for (vstream_res res; (res = ms.next_row()).success; ) {
		print_result(ELIAS_GAMMA, res);
		print_result(ELIAS_DELTA, res);
		print_result(LOG_HUFFMAN, res);
		print_result(ZLIB, res);

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
		run(numeric_limits<int32_t>::max());
	} else if (fn == "runsome") {
		run(100);
	}

	return 0;
}
