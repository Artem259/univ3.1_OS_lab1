#ifndef SERVICE_HPP
#define SERVICE_HPP

#include <iostream>
#include "src/compfuncs.hpp"
#include "src/trialfuncs.hpp"

using HARD_FAIL = os::lab1::compfuncs::hard_fail;
using SOFT_FAIL = os::lab1::compfuncs::soft_fail;

std::ostream& operator << (std::ostream &out, const std::variant<HARD_FAIL, SOFT_FAIL, int> &v)
{
    if (holds_alternative<HARD_FAIL>(v)) {
        out << std::get<HARD_FAIL>(v);
        return out;
    }
    if (holds_alternative<SOFT_FAIL>(v)) {
        out << std::get<SOFT_FAIL>(v);
        return out;
    }
    out << std::get<int>(v);
    return out;
}


#endif //SERVICE_HPP
