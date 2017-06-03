#pragma once
#ifndef INC_LIBG2FASTH_TEST_RUN_INSTANCE_H
#define INC_LIBG2FASTH_TEST_RUN_INSTANCE_H

#include "g2fasth_enums.hpp"
#include <tinythread.h>

// include fast_mutex.h disabling _FAST_MUTEX_ASM_ for Win x64 platform (because __asm is not supported)
#if (defined(_MSC_VER) && defined(_M_X64))
#define SAV_MSC_VER _MSC_VER
#undef _MSC_VER
#endif
#include "fast_mutex.h"
#ifdef SAV_MSC_VER
#define _MSC_VER SAV_MSC_VER
#endif

namespace g2 {
namespace fasth {
/**
* Stores state and outcome of specific test run instance
*/
class test_run_instance
{
public:
    /**
    * Default constructor.
    */
    test_run_instance();
    /**
    * Constructor with arguments.
    */
    test_run_instance(test_run_state, test_outcome);
    /**
    * Copy constructor.
    */
    test_run_instance(const test_run_instance& src);
    /**
    * Copy operator
    */
    test_run_instance& operator=(const test_run_instance& src);

    /**
    * Returns state of test run instance.
    * @return State of test run instance.
    */
    test_run_state state() const { return d_state; }
    /**
    * This method sets the instance state.
    */
    void set_state(test_run_state state) {
        tthread::lock_guard<tthread::fast_mutex> lg(d_mutex);
        d_state = state;
    };
    /**
    * Returns outcome of test run instance.
    * @return Outcome of test run instance.
    */
    test_outcome outcome() const { return d_outcome; }
    /**
    * This method sets the instance outcome.
    */
    void set_outcome(test_outcome outcome) {
        tthread::lock_guard<tthread::fast_mutex> lg(d_mutex);
        d_outcome = outcome;
    };

protected:
    test_run_state d_state;
    test_outcome d_outcome;
    tthread::fast_mutex d_mutex;
};
}
}

#endif // !INC_LIBG2FASTH_TEST_RUN_INSTANCE_H