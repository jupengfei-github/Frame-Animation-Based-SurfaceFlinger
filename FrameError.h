#ifndef _FRAME_ERROR_H_
#define _FRAME_ERROR_H_

#include <exception>

using namespace std;

struct base_exception : public exception {
	string desc;
	base_exception (const string exp):desc(exp) {}
	string& to_string() { return desc; }
};

struct parse_exception : public base_exception {
	parse_exception (const string exp):base_exception(exp) {}
};

struct io_exception : public base_exception {
	io_exception (const string exp):base_exception(exp) {}
};

struct structor_exception : public base_exception {
	structor_exception (const string exp):base_exception(exp) {}
};

#endif
