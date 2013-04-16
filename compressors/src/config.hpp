/*
 * config.hpp
 *
 *  Created on: Apr 10, 2013
 *      Author: mrcaps
 */

#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <unistd.h>
#include <map>

using namespace std;

class Config {
private:
	static map<const char*, const char* > opts;

	static void init_host_specific_opts(char* hostname) {
		if (0 == strcmp(hostname, "mrbox")) {
			opts["dataloc"] = "D:/";
		} else if (0 == strcmp(hostname, "GS10227")) {
			opts["dataloc"] = "G:/";
		} else {
			cerr << "Unknown host: " << hostname << endl;
			opts["dataloc"] = "/d/";
		}
	}
	static void init() {
		char* hostname = new char[128];
		gethostname(hostname, 128);
		init_host_specific_opts(hostname);
		opts["hostname"] = hostname;
		initialized = true;
	}

public:
	static bool initialized;

	static const char* get(const char* key) {
		if (!initialized) {
			init();
		}
		return opts[key];
	}
};

bool Config::initialized = false;
map<const char*, const char* > Config::opts = map<const char*, const char*>();

#endif /* CONFIG_HPP_ */
