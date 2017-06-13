#pragma once
#ifndef INC_LIBG2FASTH_TEST_RUN_SPEC_H
#define INC_LIBG2FASTH_TEST_RUN_SPEC_H

#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <iterator>
#include <assert.h>
#include "g2fasth_typedefs.hpp"
#include "test_case_graph.hpp"
#include "logger.hpp"
#include "tinythread.h"
#include <ctime>

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
class test_run_spec {
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
        , d_state(test_run_state::not_yet)
        , d_outcome(test_outcome::fail)
    {
    }
    /**
    * Constructor
    * @param test_case_action test case to be scheduled.
    * @param test_case_name test case name.
    * @param ptr_test_case pointer to test case.
    */
    test_run_spec(std::function<void(const std::string&)> action, const std::string& name, typename test_helper<T>::pmf_t ptr_test_case
        , suite<T>* suite, const chrono::milliseconds& timeout)
        : d_action(action)
        , ptr_test_case(ptr_test_case)
        , d_name(name)
        , d_after(nullptr)
        , d_after_success_of(nullptr)
        , d_after_run(nullptr)
        , d_after_success_of_run(nullptr)
        , d_suite(suite)
        , d_timeout(timeout)
        , d_state(test_run_state::not_yet)
        , d_outcome(test_outcome::fail)
    {
    }
    ~test_run_spec() {
        while (d_threads.size())
        {
            d_threads.front()->cancel();
            d_threads.pop_front();
        }
    };
    
    std::shared_ptr<test_run_spec<T>> clone(const std::string& name) const
    {
        assert(name != d_name);
        if (name == d_name)
            return nullptr;
        std::shared_ptr<test_run_spec<T>> cloned =
            std::make_shared<test_run_spec<T>>(d_action, name, ptr_test_case, d_suite, d_timeout);
        cloned->d_after = d_after;
        cloned->d_after_success_of = d_after_success_of;
        cloned->d_after_run = d_after_run;
        cloned->d_after_success_of_run = d_after_success_of_run;
        cloned->d_state = d_state;
        cloned->d_outcome = d_outcome;
        cloned->guard_condition = guard_condition;
        return cloned;
    }
    /**
    * This method schedules a test case after execution of given test case.
    * @param after_test_case name of the test to schedule after.
    * @return A test_run_spec object containing schedule of test cases.
    */
    inline test_run_spec<T>& after(typename test_helper<T>::pmf_t after_test_case) {
        if (name().empty())
        {
            throw std::invalid_argument("after can only be called after scheduling a test case.");
        }
        if (get_ptr_test_case() == after_test_case)
        {
            throw std::invalid_argument("same test case cannot be scheduled as dependent of itself.");
        }
        assert(d_suite != nullptr);
        if (!d_suite->test_case_exists(after_test_case))
        {
            throw std::invalid_argument("after can only be called after scheduling a test case.");
        }
        {
            tthread::lock_guard<tthread::mutex> lg(d_mutex);
            d_after = after_test_case;
        }
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
        if (name().empty())
        {
            throw std::invalid_argument("after can only be called after scheduling a test case.");
        }
        if (get_ptr_test_case() == spec.get_ptr_after_test_case())
        {
            throw std::invalid_argument("same test case cannot be scheduled as dependent of itself.");
        }
        if (ptr_test_case == spec.get_ptr_after_test_case())
        {
            throw std::invalid_argument("same test case cannot be scheduled as dependent of itself.");
        }
        {
            tthread::lock_guard<tthread::mutex> lg(d_mutex);
            d_after_run = &spec;
        }
        d_suite->schedule_before(spec.get_ptr_test_case(), ptr_test_case);
        return *this;
    }
    /**
    * This method schedules a test case after success of execution given test case.
    * @param after_test_case name of the test to schedule after.
    * @return A test_run_spec object containing schedule of test cases.
    */
    inline test_run_spec& after_success_of(typename test_helper<T>::pmf_t after_test_case) {
        if (name().empty())
        {
            throw std::invalid_argument("after_success_of can only be called after scheduling a test case.");
        }
        if (get_ptr_test_case() == after_test_case)
        {
            throw std::invalid_argument("same test case cannot be scheduled as dependent of itself.");
        }
        assert(d_suite != nullptr);
        if (!d_suite->test_case_exists(after_test_case))
        {
            throw std::invalid_argument("after_success_of can only be called after scheduling a test case.");
        }
        {
            tthread::lock_guard<tthread::mutex> lg(d_mutex);
            d_after_success_of = after_test_case;
        }
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
        if (name().empty())
        {
            throw std::invalid_argument("after can only be called after scheduling a function.");
        }
        if (get_ptr_test_case() == spec.get_ptr_after_test_case())
        {
            throw std::invalid_argument("same test case cannot be scheduled as dependent of itself.");
        }
        {
            tthread::lock_guard<tthread::mutex> lg(d_mutex);
            d_after_success_of_run = &spec;
        }
        d_suite->schedule_before(spec.get_ptr_test_case(), ptr_test_case);
        return *this;
    }
    /**
    * This method sets a guard condition to be executed before test case is executed.
    * @param argument1 guard condition to be executed.
    * @return A test_run_spec object containing schedule of test cases.
    */
    inline test_run_spec& guard(std::function<bool()> func) {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        guard_condition = func;
        return *this;
    }
    /**
    * This method executes test cases setup in test track.
    * It validates every condition before executing a test case.
    */
    inline bool execute(std::function<void(const std::string&)> async_func_obj=nullptr
        , const chrono::milliseconds& timeout=chrono::milliseconds(0)) {
        if (async_func_obj)
        {
            tthread::lock_guard<tthread::mutex> lg(d_mutex);
            d_action = async_func_obj;
            d_timeout = timeout;
        }

        std::shared_ptr<tthread::thread> thread;
        {
            tthread::lock_guard<tthread::mutex> lg(d_mutex);
            if (d_state == test_run_state::not_yet)
                d_state = test_run_state::ongoing;
            // Run test case body in separate thread to have possibility of time measurement
            thread = std::make_shared<tthread::thread>(action_thread_proc, this);
            d_threads.push_back(thread);
        }

        // Wait for the thread completion during the timeout
        bool test_done;
        bool timed_out = !thread->join(d_timeout);
        {
            tthread::lock_guard<tthread::mutex> lg(d_mutex);
            if (timed_out)
            {   // Timed out. We don't cancel the thread here. We do this in destructor.
                if (d_state == test_run_state::done)
                    // The test called complete_test_case() before timed out. So mean test timed in.
                    timed_out = false;
                else
                    internal_complete(test_outcome::fail);
            }
            test_done = d_state == test_run_state::done;
        }
        if (timed_out)
            d_suite->get_logger().log(log_level::REGULAR, "Test case '" + name() + "' is timed out");
        return test_done;
    }
    /**
    * This method returns name of test case.
    * @return name of test case.
    */
    const std::string& name() {
        return d_name;
    }
    /**
    * Returns pointer to test case.
    * @return pointer to test case.
    */
    typename test_helper<T>::pmf_t get_ptr_test_case() const {
        return ptr_test_case;
    }
    /**
    * Returns pointer to test case after which this test case can run.
    * @return pointer to test case.
    */
    typename test_helper<T>::pmf_t get_ptr_after_test_case() const {
        return d_after;
    }

    /**
    * Returns pointer to test case after success of which this test case can run.
    * @return pointer to test case.
    */
    typename test_helper<T>::pmf_t get_ptr_after_success_of_test_case() const {
        return d_after_success_of;
    }

    bool valid_to_execute() {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
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
        if (guard_condition)
        {
            lg.~lock_guard();
            if (!guard_condition())
                complete(test_outcome::fail);
            return false;
        }
        // Test case has passed pre-conditions and ready to execute.
        return true;
    }

    test_run_state state() const {
        return d_state; 
    }
    /**
    * This method sets the instance state.
    */
    void set_state(test_run_state state) {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        d_state = state;
    };
    /**
    * Returns outcome of test run instance.
    * @return Outcome of test run instance.
    */
    test_outcome outcome() const {
        return d_outcome; 
    }
    /**
    * This method completes the test.
    */
    void complete(test_outcome outcome) {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        internal_complete(outcome);
    }

private:
    void internal_complete(test_outcome outcome) {
        if (d_state == test_run_state::done)
            return;
        d_outcome = outcome;
        d_state = test_run_state::done;
    };
    bool validate_after_success_of(const test_run_spec<T>& d_after_success_of_test_case) {
        // and other test case is not done or yet to start, return.
        if (d_after_success_of_test_case.state() != test_run_state::done)
            return false;
        // and other test case has failed, we fail this test case too and return.
        if (d_after_success_of_test_case.outcome() == test_outcome::fail)
        {
            internal_complete(test_outcome::fail);
            return false;
        }
        return true;
    }


    // Thread procedure for test action run
    static void action_thread_proc(void* p) {
        tthread::thread::make_cancel_safe();
        test_run_spec* _this = (test_run_spec*)p;
        try {
            _this->d_action(_this->d_name);
        }
        catch(...) {
            _this->complete(test_outcome::fail);
        }
    }

private:
    std::list<std::shared_ptr<tthread::thread>> d_threads;
    tthread::mutex d_mutex;
    std::string d_name;
    test_run_state d_state;
    test_outcome d_outcome;
    typename test_helper<T>::pmf_t ptr_test_case;
    std::function<void(const std::string&)> d_action;
    typename test_helper<T>::pmf_t d_after;
    typename test_helper<T>::pmf_t d_after_success_of;
    test_run_spec* d_after_run;
    test_run_spec* d_after_success_of_run;
    std::function<bool()> guard_condition;
    suite<T>* d_suite;
    chrono::milliseconds d_timeout;
};
}
}

#endif // !INC_LIBG2FASTH_TEST_RUN_SPEC_H