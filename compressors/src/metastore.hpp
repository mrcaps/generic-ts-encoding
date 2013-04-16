/*
 * metastore.hpp
 *
 *  Created on: Apr 9, 2013
 *      Author: ishafer
 */

#ifndef METASTORE_HPP_
#define METASTORE_HPP_

#include "../inc/sqlite3.h"
#include <cstring>
#include <cassert>
#include <cstdio>
#include <cstdlib>

#include <vector>

#include "config.hpp"

using namespace std;

static int check_exec(sqlite3* db, const char *sql) {
	char *local_err;
	if (sqlite3_exec(db, sql, NULL, NULL, &local_err) != SQLITE_OK) {
		cerr << local_err << endl;
		return 1;
	}
	return 0;
}

/**
 * A value stream metadata entry
 */
struct vstream {
	char *vname;
	char *tpath;
	char *vpath;
	int vmin;
	int vmax;
	int vscale;
	int vsize;
	int npoints;
};
struct vstream_res {
	vstream p;
	bool success;
};

class MetaStore {
private:
	const char* fname;
	const char* prefix_replace;
	int len_prefix_replace;
	sqlite3 *db;
	sqlite3_stmt *stmt;

public:
	/**
	 * @brief new metadata store
	 * @param _fname input DB file name
	 * @param _prefix_replace (a bit of a hack) replace the prefix of all
	 *  paths with this one
	 */
	MetaStore(const char* _fname, const char* _prefix_replace) :
		fname(_fname),
		prefix_replace(_prefix_replace),
		db(NULL),
		stmt(NULL)
	{
		len_prefix_replace = strlen(prefix_replace);
	}

	void open() {
		int rc = sqlite3_open(fname, &db);
		if (rc) {
			cerr << "couldn't open db" << sqlite3_errmsg(db) << endl;
			close();
		}
	}

	void check(int rc) {
		if (rc != SQLITE_OK) {
			cerr << "Failed with status " << rc << ":" << sqlite3_errmsg(db) << endl;
		}
	}

	void close() {
		sqlite3_close(db);
	}

	void start_iter() {
		const char *sql = "select vname, tpath, vpath, vmin, vmax, vscale, vsize, npoints from meta;";
		check(sqlite3_prepare_v2(db, sql, strlen(sql)+1, &stmt, NULL));
	}

	vstream_res next_row() {
		vstream_res res;
		res.success = false;
		int rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW) {
			res.p.vname = strdup(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
			res.p.tpath = strdup(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
			memcpy(res.p.tpath, prefix_replace, len_prefix_replace - 1);
			res.p.vpath = strdup(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
			memcpy(res.p.vpath, prefix_replace, len_prefix_replace - 1);

			res.p.vmin = sqlite3_column_int(stmt, 3);
			res.p.vmax = sqlite3_column_int(stmt, 4);
			res.p.vscale = sqlite3_column_int(stmt, 5);
			res.p.vsize = sqlite3_column_int(stmt, 6);
			res.p.npoints = sqlite3_column_int(stmt, 7);
			res.success = true;
		} else if (rc == SQLITE_DONE) {
			sqlite3_finalize(stmt);
		} else {
			cerr << "Couldn't get next row" << endl;
		}
		return res;
	}
};

/**
 * @param vs value stream
 * @returns ptr to allocated contents
 */
void* read_fully(vstream vs) {
	FILE *fp = fopen(vs.vpath, "rb");
	void *space = NULL;
	if (NULL == fp) {
		cerr << "Couldn't open path " << vs.vpath << endl;
	} else {
		space = malloc(vs.npoints * vs.vsize);
		fread(space, vs.vsize, vs.npoints, fp);
	}

	fclose(fp);
	return space;
}

/**
 * Check some values from stream 1
 */
static void test_stream1(vstream vs) {
	int32_t* arr = (int32_t*) read_fully(vs);
	assert(arr[0] == 21367500);
	assert(arr[1] == 23076900);
	assert(arr[2] == 24786300);
	assert(arr[3] == 23076900);

	assert(arr[vs.npoints-2] == 17948700);
	assert(arr[vs.npoints-1] == 16239300);
}

/**
 * ... and from stream 2
 */
static void test_stream2(vstream vs) {
	int32_t* arr = (int32_t*) read_fully(vs);
	assert(arr[0] == 0);
	assert(arr[1] == 181817);
	assert(arr[2] == 363636);
	assert(arr[3] == 545454);

	assert(arr[vs.npoints-2] == 0);
	assert(arr[vs.npoints-1] == -181817);
}

/**
 * ... and from stream 3
 */
static void test_stream3(vstream vs) {
	int* arr = (int*) read_fully(vs);
	assert(arr[0] == -910714);
	assert(arr[1] == -946428);
	assert(arr[2] == -964286);
	assert(arr[3] == -964286);

	assert(arr[vs.npoints-2] == -964286);
	assert(arr[vs.npoints-1] == -982143);
}

static const char* testdbs[] = {
		"../testdata/cmu-robot-field/meta.db",
		"../testdata/inline-skating/meta.db"
};
static const unsigned N_TEST_DBS = 2;

void test_metastore() {
	MetaStore ms = MetaStore(testdbs[0], Config::get("dataloc"));

	ms.open();
	ms.start_iter();
	int points = 0;
	for (vstream_res res; (res = ms.next_row()).success; ) {
		//cout << "got point " << res.p.vpath << endl;
		assert(res.p.vsize == 4);
		assert(res.p.npoints == 30282);
		if (0 == points) {
			test_stream1(res.p);
		} else if (1 == points) {
			test_stream2(res.p);
		} else if (2 == points) {
			test_stream3(res.p);
		}
		++points;
	}
	ms.close();

	assert(points == 3);
}

vstream get_test_stream() {
	MetaStore ms(testdbs[0], Config::get("dataloc"));

	ms.open();
	ms.start_iter();

	vstream_res res = ms.next_row();
	assert(res.success);

	ms.close();

	return res.p;
}

vector<vstream> get_test_streams() {
	vector<vstream> vstreams;

	for (unsigned dx = 0; dx < N_TEST_DBS; ++dx) {
		MetaStore ms(testdbs[dx], Config::get("dataloc"));
		ms.open();
		ms.start_iter();

		vstream_res res = ms.next_row();

		for (vstream_res res; (res = ms.next_row()).success; ) {
			vstreams.push_back(res.p);
		}

		ms.close();
	}

	return vstreams;
}

#endif /* METASTORE_HPP_ */
