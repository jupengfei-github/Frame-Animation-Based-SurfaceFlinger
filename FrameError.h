/*
 * Copyright (C) 2018-2024 The Surface Frame-Animation Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
