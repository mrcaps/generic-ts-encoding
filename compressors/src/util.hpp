/*
 * util.hpp
 * @brief a few global utilities
 * @author ishafer
 */

#ifndef UTIL_HPP_
#define UTIL_HPP_

#undef DISALLOW_EVIL_CONSTRUCTORS
#define DISALLOW_EVIL_CONSTRUCTORS(TypeName) \
   TypeName(const TypeName&); \
   void operator=(const TypeName&)

#include <iostream>

using namespace std;

template <typename T>
void print_arr(T* arr, uint64_t len) {
	cout << "[";
	for (uint64_t i = 0; i < len - 1; ++i) {
		cout << (int64_t) arr[i] << ", ";
	}
	if (len > 0) {
		cout << (int64_t) arr[len-1];
	}
	cout << "]";
}


#endif /* UTIL_HPP_ */
