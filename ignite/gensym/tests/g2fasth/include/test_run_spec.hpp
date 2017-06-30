#pragma once
#ifndef INC_LIBG2FASTH_TEST_RUN_SPEC_H
#define INC_LIBG2FASTH_TEST_RUN_SPEC_H

#include <memory>
#include <vector>
#include <map>
#include <list>
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
template <class T>
class test_run_spec;

template <class T>
struct async_run_data
{
    suite<T>* test_suite;
    test_run_spec<T>* test_case;
    std::string test_case_name;
    std::function<void(const std::string&)> func_obj;
    typename test_helper<T>::pmf_t user_func_ptr;
    int interval;
};
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
        if (d_name.empty())
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
        if (d_name.empty())
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
        if (d_name.empty())
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
        if (d_name.empty())
        {
            throw std::invalid_argument("after can only be called after scheduling a function.");
        }
        if (ptr_test_case == spec.get_ptr_after_test_case())
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
    bool is_timeout(bool lock=true, int* elapsed=nullptr)
    {
        clock_t now = clock();
        clock_t start;
        int timeout;
        test_run_state tstate;
        if (lock)
            d_mutex.lock();
        tstate = d_state;
        start = d_start;
        timeout = (int)d_timeout.count();
        if (lock)
            d_mutex.unlock();
        if (tstate != test_run_state::ongoing)
            return false;
        int elapsed_ms = int(double(now - start) / CLOCKS_PER_SEC * 1000 + 0.5);
        printf("is_timeout(%s): elapsed=%d\n", lock?"ext":"int", elapsed_ms);
        if (elapsed)
            *elapsed = elapsed_ms;
        return elapsed_ms >= timeout;
    }
    /**
    * This method executes test cases setup in test track.
    * It validates every condition before executing a test case.
    */
    inline bool execute(async_run_data<T>* async_data=nullptr) {
        FUNCLOG2(d_name+(async_data?"-ASYNC":""));
        std::unique_ptr<async_run_data<T>> data(async_data);
        std::shared_ptr<tthread::thread> thread;
        chrono::milliseconds timeout(0);
        int elapsed = 0;
        {
            tthread::lock_guard<tthread::mutex> lg(d_mutex);
            if (!data)
            {
                data.reset(new async_run_data<T>);
                data->test_case_name = d_name;
                data->func_obj = d_action;
                data->user_func_ptr = nullptr;
                data->interval = 0;
                timeout = d_timeout;
                d_state = test_run_state::ongoing;
                d_start = clock();
            }
            else
            {
                if (is_timeout(false, &elapsed))
                {
                    internal_complete(test_outcome::fail);
                    return true;
                }
                timeout = chrono::milliseconds(d_timeout.count() - elapsed);
            }
            data->test_case = this;
            // Run test case body in separate thread to have possibility of time measurement
            thread = std::make_shared<tthread::thread>(action_thread_proc, data.release());
            d_threads.push_back(thread);
        }

        // Wait for the thread completion during the timeout
        bool test_done;
        bool timed_out = !thread->join(timeout);
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
    typename test_helper<T>::pmf_t get_ptr_after_test_case() {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        return d_after;
    }

    /**
    * Returns pointer to test case after success of which this test case can run.
    * @return pointer to test case.
    */
    typename test_helper<T>::pmf_t get_ptr_after_success_of_test_case() {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
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

    test_run_state state() {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
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
    test_outcome outcome() {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        return d_outcome;
    }
    /**
    * This method completes the test.
    */
    void complete(test_outcome outcome) {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        internal_complete(outcome);
    }

    void stop_timer(typename test_helper<T>::pmf_t func_ptr)
    {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        auto it = std::find(d_stop_timers.begin(), d_stop_timers.end(), func_ptr);
        if (it != d_stop_timers.end())
            return; // Already in list
        d_stop_timers.push_back(func_ptr);
    }

private:
    bool is_timer_stopped(typename test_helper<T>::pmf_t func_ptr)
    {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        auto it = std::find(d_stop_timers.begin(), d_stop_timers.end(), func_ptr);
        bool stop = it != d_stop_timers.end();
        if (stop)
            d_stop_timers.remove(func_ptr);
        return stop || (d_state != test_run_state::ongoing) || is_timeout(false);
    }
    void internal_complete(test_outcome outcome) {
        if (d_state == test_run_state::done)
            return;
        d_outcome = outcome;
        d_state = test_run_state::done;
    };
    bool validate_after_success_of(test_run_spec<T>& d_after_success_of_test_case) {
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
        std::unique_ptr<async_run_data<T>> data((async_run_data<T>*)p);
        FUNCLOG2(data->test_case_name);
        test_run_spec* _this = data->test_case;
        try {
            int interval = data->interval;
            if (interval > 0)
            {   // timer
                int action_elapsed_ms = 0;
                do 
                {
                    int next_interval = interval - action_elapsed_ms;
                    SCOPELOG(data->test_case_name+" - iteration, next interval="+std::to_string((long long)next_interval));
                    bool stop;
                    // Sleep between action calls with state checking
                    while (!(stop = _this->is_timer_stopped(data->user_func_ptr)))
                    {
                        if (next_interval <= 0)
                            break;
                        int sleep_time = next_interval<50 ? next_interval : 50;
                        tthread::this_thread::sleep_for(tthread::chrono::milliseconds(sleep_time));
                        next_interval -= sleep_time;
                    }
                    if (stop)
                        break;
                    clock_t begin = clock();
                    data->func_obj(data->test_case_name);
                    action_elapsed_ms = (int)(double(clock() - begin) / CLOCKS_PER_SEC * 1000);
                } while(true);
            }
            else
            {   // simple async
                data->func_obj(data->test_case_name);
            }
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
    clock_t d_start;
    std::list<typename test_helper<T>::pmf_t> d_stop_timers;
};
}
}

#endif // !INC_LIBG2FASTH_TEST_RUN_SPEC_H