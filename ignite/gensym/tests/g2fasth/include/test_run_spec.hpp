#pragma once
#ifndef INC_LIBG2FASTH_TEST_RUN_SPEC_H
#define INC_LIBG2FASTH_TEST_RUN_SPEC_H

#include <vector>
#include <map>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <iterator>
#include <assert.h>
#include "test_run_instance.hpp"
#include "g2fasth_typedefs.hpp"
#include "test_case_graph.hpp"
#include <tinythread.h>
#include <ctime>
#include <time.h>
#include <stdlib.h>

class ScopeLog
{
  std::string _name;
public:
  ScopeLog(const std::string& ame) : _name( ame)
  {
    print(_name + " ENTER");
  }
  ~ScopeLog()
  {
    print(_name + " LEAVE");
  }

  static void print(const char* line)
  {
      int sec, msec;
      get_sec_msec(&sec, &msec);
      printf("%08d:%03d %s\n", sec, msec, line);
  }
  static void print(const std::string& line)
  {
    print(line.c_str());
  }
};

#define FUNCLOG ScopeLog _sl_(__FUNCTION__)


namespace g2 {
namespace fasth {

namespace chrono = tthread::chrono;

template <class T>
class suite;
/**
* This class is responsible storing relationships with other tests.
* A test case can be scheduled after a test case, after success of a test case
* as well as guard conditions be added before the execution of test case.
*/
template <class T>
class test_run_spec : public test_run_instance {
public :
    /**
    * Default constructor.
    * This is only used by unit test cases for testing purposes.
    */
    test_run_spec() :
        d_after(nullptr)
        , d_after_success_of(nullptr)
        , d_after_run(nullptr)
        , d_after_success_of_run(nullptr)
        , d_suite(nullptr)
        , d_timeout(0)
    {
    }
    /**
    * Constructor
    * @param test_case_action test case to be scheduled.
    * @param test_case_name test case name.
    * @param ptr_test_case pointer to test case.
    */
    test_run_spec(std::function<test_outcome(g2::fasth::test_run_instance&)> test_case_action, std::string test_case_name, typename g2::fasth::test_helper<T>::pmf_t ptr_test_case
        , g2::fasth::suite<T> & test_suite, const chrono::milliseconds& timeout)
        : d_action(test_case_action)
        , ptr_test_case(ptr_test_case)
        , d_test_case_name(test_case_name)
        , d_after(nullptr)
        , d_after_success_of(nullptr)
        , d_after_run(nullptr)
        , d_after_success_of_run(nullptr)
        , d_suite(&test_suite)
        , d_timeout(timeout)
    {
    }
    ~test_run_spec() {};
    /**
    * This method schedules a test case after execution of given test case.
    * @param after_test_case name of the test to schedule after.
    * @return A test_run_spec object containing schedule of test cases.
    */
    inline test_run_spec<T>& after(typename g2::fasth::test_helper<T>::pmf_t after_test_case) {
        if (d_test_case_name.empty())
        {
            throw std::invalid_argument("after can only be called after scheduling a test case.");
        }
        if (ptr_test_case == after_test_case)
        {
            throw std::invalid_argument("same test case cannot be scheduled as dependent of itself.");
        }
        assert(d_suite != nullptr);
        if (!d_suite->test_case_exists(after_test_case))
        {
            throw std::invalid_argument("after can only be called after scheduling a test case.");
        }
        d_after = after_test_case;
        d_suite->schedule_before(after_test_case, ptr_test_case);
        if (d_suite->validate_cycle())
        {
            throw std::invalid_argument("the given schedule is creating a cycle.");
        }
        return *this;
    }
    /**
    * This method schedules a test case after execution of given test spec.
    * @param spec test_run_spec object, in which test case is to be scheduled.
    * @return A test_run_spec object containing schedule of test cases.
    */
    inline test_run_spec& after(test_run_spec& spec) {
        if (d_test_case_name.empty())
        {
            throw std::invalid_argument("after can only be called after scheduling a test case.");
        }
        if (ptr_test_case == spec.get_ptr_after_test_case())
        {
            throw std::invalid_argument("same test case cannot be scheduled as dependent of itself.");
        }
        if (ptr_test_case == spec.get_ptr_after_test_case())
        {
            throw std::invalid_argument("same test case cannot be scheduled as dependent of itself.");
        }
        d_after_run = &spec;
        d_suite->schedule_before(spec.get_ptr_test_case(), ptr_test_case);
        return *this;
    }
    /**
    * This method schedules a test case after success of execution given test case.
    * @param after_test_case name of the test to schedule after.
    * @return A test_run_spec object containing schedule of test cases.
    */
    inline test_run_spec& after_success_of(typename g2::fasth::test_helper<T>::pmf_t after_test_case) {
        if (d_test_case_name.empty())
        {
            throw std::invalid_argument("after_success_of can only be called after scheduling a test case.");
        }
        if (ptr_test_case == after_test_case)
        {
            throw std::invalid_argument("same test case cannot be scheduled as dependent of itself.");
        }
        assert(d_suite != nullptr);
        if (!d_suite->test_case_exists(after_test_case))
        {
            throw std::invalid_argument("after_success_of can only be called after scheduling a test case.");
        }
        d_after_success_of = after_test_case;
        d_suite->schedule_before(after_test_case, ptr_test_case);
        if (d_suite->validate_cycle())
        {
            throw std::invalid_argument("the given schedule is creating a cycle.");
        }
        return *this;
    }
    /**
    * This method schedules a test case after success of execution of given test spec.
    * @param argument1 test_run_spec object, in which test case is to be scheduled.
    * @return A test_run_spec object containing schedule of test cases.
    */
    inline test_run_spec& after_success_of(test_run_spec& spec) {
        if (d_test_case_name.empty())
        {
            throw std::invalid_argument("after can only be called after scheduling a function.");
        }
        if (ptr_test_case == spec.get_ptr_after_test_case())
        {
            throw std::invalid_argument("same test case cannot be scheduled as dependent of itself.");
        }
        d_after_success_of_run = &spec;
        d_suite->schedule_before(spec.get_ptr_test_case(), ptr_test_case);
        return *this;
    }
    /**
    * This method sets a guard condition to be executed before test case is executed.
    * @param argument1 guard condition to be executed.
    * @return A test_run_spec object containing schedule of test cases.
    */
    inline test_run_spec& guard(std::function<bool()> func) {
        guard_condition = func;
        return *this;
    }
    /**
    * This method executes test cases setup in test track.
    * It validates every condition before executing a test case.
    */
    inline bool execute() {
        auto execute_test_case = true;
        if (d_after_run != nullptr)
        {
            if (d_after_run->state() != test_run_state::done)
            {
                execute_test_case = false;
            }
        }
        if (d_after_success_of_run != nullptr)
        {
            if (d_after_success_of_run->outcome() != test_outcome::pass)
            {
                execute_test_case = false;
            }
        }
        if (guard_condition && !guard_condition())
        {
            execute_test_case = false;
        }
        if (execute_test_case)
        {
            set_state(test_run_state::ongoing);

            char buf[10];
            ScopeLog sl(d_test_case_name + " execute, timeout = " + itoa(d_timeout.count(),buf,10));

            // Run test case body in separate thread to have
            // possibility of time measurement and test canceling
            tthread::thread thread(action_thread_proc, this);
            // Wait for the thread completion during the timeout
            if (!thread.join(d_timeout))
            {   // Timed out, so
                ScopeLog::print(d_test_case_name + " killed");
                // cancel the test
                thread.cancel();
                // set outcome as fail
                set_outcome(g2::fasth::test_outcome::fail);
            }
            else
            {   // Completed in time (sync body or async starter)
                if (outcome() == g2::fasth::test_outcome::by_instance)
                {   // Test went async
                    return true;
                }
            }

            set_state(test_run_state::done);
        }
        else
        {
            set_outcome(g2::fasth::test_outcome::fail);
        }
        return false;
    }
    /**
    * This method returns name of test case.
    * @return name of test case.
    */
    const std::string& name() {
        return d_test_case_name;
    }
    /**
    * This method sets name of test case.
    */
    void set_name(const std::string& name) {
        d_test_case_name = name;
    }
    /**
    * Returns pointer to test case.
    * @return pointer to test case.
    */
    typename g2::fasth::test_helper<T>::pmf_t get_ptr_test_case() const {
        return ptr_test_case;
    }
    /**
    * Returns pointer to test case after which this test case can run.
    * @return pointer to test case.
    */
    typename g2::fasth::test_helper<T>::pmf_t get_ptr_after_test_case() const {
        return d_after;
    }

    test_outcome execution_outcome() {
        return d_async_instance.outcome();
    }
    /**
    * Returns pointer to test case after success of which this test case can run.
    * @return pointer to test case.
    */
    typename g2::fasth::test_helper<T>::pmf_t get_ptr_after_success_of_test_case() const {
        return d_after_success_of;
    }

    bool valid_to_execute() {
        // If current test case depends on some other test, and other test case is not done, return.
        // We will try to execute this again in next iteration.
        if (d_after != nullptr && d_suite->get_spec(d_after).state() != test_run_state::done)
        {
            return false;
        }
        else if (d_after_run != nullptr)
        {
            auto ptr_test_case = d_after_run->get_ptr_test_case();
            if (d_suite->get_spec(ptr_test_case).state() != test_run_state::done)
            {
                return false;
            }
        }
        // If current test case depends on success of some other test
        if (d_after_success_of != nullptr)
        {
            return validate_after_success_of(d_suite->get_spec(d_after_success_of));
        }
        else if (d_after_success_of_run != nullptr)
        {
            auto ptr_test_case = d_after_success_of_run->get_ptr_test_case();

            return validate_after_success_of(d_suite->get_spec(ptr_test_case));
        }
        // If current test case depends on some guard condition, and that condition fails,
        // we fail this test case too and return.
        if (guard_condition && !guard_condition())
        {
            d_state = test_run_state::done;
            set_outcome(test_outcome::fail);
            return false;
        }
        // Test case has passed pre-conditions and ready to execute.
        return true;
    }
private:
    typename g2::fasth::test_helper<T>::pmf_t ptr_test_case;
    std::function<g2::fasth::test_outcome(g2::fasth::test_run_instance &)> d_action;
    std::string d_test_case_name;
    typename g2::fasth::test_helper<T>::pmf_t d_after;
    typename g2::fasth::test_helper<T>::pmf_t d_after_success_of;
    test_run_spec* d_after_run;
    test_run_spec* d_after_success_of_run;
    std::function<bool()> guard_condition;
    g2::fasth::suite<T> * d_suite;
    g2::fasth::test_run_instance d_async_instance;
    chrono::milliseconds d_timeout;

    bool validate_after_success_of(g2::fasth::test_run_spec<T> d_after_success_of_test_case) {
        // and other test case is not done or yet to start, return.
        if (d_after_success_of_test_case.state() == test_run_state::ongoing
            || d_after_success_of_test_case.state() == test_run_state::not_yet)
        {
            return false;
        }
        // and other test case has failed, we fail this test case too and return.
        else if (d_after_success_of_test_case.outcome() != test_outcome::pass)
        {
            d_state = test_run_state::done;
            set_outcome(test_outcome::fail);
            return false;
        }
        return true;
    }

    // Thread procedure for test action run
    static void action_thread_proc(void* p) {
        tthread::thread::make_cancel_safe();
        test_run_spec* _this = (test_run_spec*)p;
        ScopeLog sl(_this->d_test_case_name + " thread");
        g2::fasth::test_outcome outcome;
        try {
            outcome = _this->d_action(_this->d_async_instance);
        }
        catch(...) {
            outcome = g2::fasth::test_outcome::fail;
        }
        _this->set_outcome(outcome);
    }
};
}
}

#endif // !INC_LIBG2FASTH_TEST_RUN_SPEC_H