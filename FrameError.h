#ifndef _FRAME_ERROR_H_
#define _FRAME_ERROR_H_

#include <exception>

using namespace std;

struct parse_exception : public exception {
	string desc;
	parse_exception (const string exp):desc(exp) {}
	string& to_string() { return desc; }
};

#endif
