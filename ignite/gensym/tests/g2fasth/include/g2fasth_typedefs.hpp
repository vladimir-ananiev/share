#pragma once
#ifndef INC_LIBG2FASTH_TYPEDEFS_H
#define INC_LIBG2FASTH_TYPEDEFS_H

#include "g2fasth_enums.hpp"

namespace g2 {
namespace fasth {
/**
* Helper struct for placeholder of member function pointer.
* This should be replaced with alias on upgradation.
*/
template <typename T>
struct test_helper {
    typedef void(T::*pmf_t)(const std::string&,test_run_reason);
};
}
}
#endif // !INC_LIBG2FASTH_TYPEDEFS_H