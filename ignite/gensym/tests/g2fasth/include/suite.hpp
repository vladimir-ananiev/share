#pragma once
#ifndef INC_LIBG2FASTH_SUITE_H
#define INC_LIBG2FASTH_SUITE_H

#include <list>
#include <vector>
#include <iostream>
#include <string>
#include <stdexcept>
#include <iterator>
#include <fstream>
#include <ios>
#include "g2fasth_enums.hpp"
#include "test_run_spec.hpp"
#include "g2fasth_typedefs.hpp"
#include "junit_report.hpp"
#include "base_suite.hpp"

namespace g2 {
namespace fasth {

/**
* Stores relationship between a test case and its outcome.
*/
struct test_result {
    test_result(std::string test_case_name, test_outcome test_outcome)
        : d_test_case_name(test_case_name), d_test_outcome(test_outcome)
    {
    }
    std::string test_case_name() {
        return d_test_case_name;
    }
    test_outcome outcome() {
        return d_test_outcome;
    }
private:
    std::string d_test_case_name;
    test_outcome d_test_outcome;
};

// Max timeout for tests
#define G2FASTH_MAX_TIMEOUT      7*24*3600*1000  // 1 week
// Default timeout for tests
#define G2FASTH_DEFAULT_TIMEOUT  600*1000        // 10 min

template <class T>
class suite : public base_suite {
public:
    /**
    * Constructor that sets the name of the suite, its execution order and log level.
    * @param suite_name name of the test suite.
    * @param test_order order of execution of test cases.
    * @param logger_file_path absolute path to the log file.
    * @param suite_log_level log level of test suite.
    */
    suite(std::string suite_name, test_order test_order,
        log_level suite_log_level, std::string logger_file_path = "", 
        const chrono::milliseconds& default_timeout=chrono::milliseconds(G2FASTH_DEFAULT_TIMEOUT))
        : base_suite(suite_name)
        , d_order(test_order)
        , d_logger(suite_log_level)
        , d_default_timeout(default_timeout)
    {
        if (d_default_timeout.count() > G2FASTH_MAX_TIMEOUT)
            d_default_timeout = chrono::milliseconds(G2FASTH_MAX_TIMEOUT);
        d_logger.add_output_stream(std::cout, log_level::REGULAR);
        if (!logger_file_path.empty())
        {
            d_logger.add_output_stream(new std::ofstream(logger_file_path, std::ios_base::app), log_level::VERBOSE);
        }
    }
    ~suite()
    {
        int cancelled = 0;
        while (d_threads_to_cancel.size())
        {
            if (d_threads_to_cancel.front()->cancel())
                cancelled++;
            d_threads_to_cancel.pop_front();
        }
        clock_t start = clock();
        while (d_threads_to_wait.size())
        {
            d_threads_to_wait.front()->join();
            d_threads_to_wait.pop_front();
        }
#ifndef WIN32
        if (cancelled)
        {   // Let cancelled threads (in Linux) to stop
            //puts("Wait for cancelled threads...");
            int elapsed = int(double(clock() - start) / CLOCKS_PER_SEC * 1000 + 0.5);
            int sleep = 10000 - elapsed;
            if (sleep > 0)
            {
                printf("Wait % ms...\n", sleep);
                tthread::this_thread::sleep_for(tthread::chrono::milliseconds(sleep));
            }
        }
#endif
    }
    /**
    * This function executes test suite. First it setups test track and then starts execution.
    * @param report_file_name Absolute path of the JUnit report xml, can be empty if report is not to be written
    * @return JUnit report in string format.
    */
    inline std::string execute(std::string report_file_name = "") override {
        int count;
        {
            tthread::lock_guard<tthread::mutex> lg(d_mutex);
            count = d_test_specs.size();
        }
        if (count == 0)
        {
            d_logger.log(log_level::SILENT, "Setting up test track for test suite : " + get_suite_name());
            setup_test_track();
            d_logger.log(log_level::SILENT, "Test track setup done for test suite : " + get_suite_name());
        }
        return start(report_file_name);
    }

    /**
    * This function schedules a test case for execution.
    * @param ptr_test_case pointer to test case to be executed.
    * @param test_case_name name of test case.
    * @param timeout timeout of test case.
    * @return Handle of created test run instance.
    */
    inline test_run_spec<T>& run(typename test_helper<T>::pmf_t ptr_test_case
            , const std::string& test_case_name, const chrono::milliseconds& timeout=chrono::milliseconds(0)) {
        std::function<void(const std::string&)> action =
            std::bind(ptr_test_case, static_cast<T*>(this), std::placeholders::_1);
        auto ptr = schedule(action, test_case_name, ptr_test_case, timeout);
        assert(ptr != nullptr);
        return *(ptr.get());
    }
    /**
    * Returns test run instance by name.
    * @param name Name of test run instance to return.
    * @return Handle of specified test run instance.
    */
    inline test_run_spec<T>& instance(const std::string& name) {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        auto it = std::find_if(d_test_specs.begin(), d_test_specs.end(), [&](std::shared_ptr<test_run_spec<T>> spec) {
            return spec->name() == name;
        });
        if (it == d_test_specs.end())
        {
            throw std::invalid_argument("Instance " + name + " doesn't exist.");
        }
        return *(it->get());
    }
    /**
    * This function returns an instance of logger which can be used to log any data.
    * @return An instance of logger.
    */
    inline logger& get_logger() {
        return d_logger;
    }
    /**
    * This function return true or false depending on if test case exists in schedule.
    * @param test_case test case existence for which is to be verified.
    * @return true or false.
    */
    inline bool test_case_exists(typename test_helper<T>::pmf_t test_case) {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        return internal_test_case_exists(test_case);
    }
    /**
    * This function returns instance of spec.
    * @param ptr_test_case pointer to test case for which test spec is to be returned.
    * @return instance of test spec.
    */
    test_run_spec<T>& get_spec(typename test_helper<T>::pmf_t test_case) {
        return *(std::find_if(d_test_specs.begin(), d_test_specs.end(), [&](std::shared_ptr<test_run_spec<T>> spec) {
            return spec->get_ptr_test_case() == test_case;
        })->get());
    }
    /**
    * This function validates if there exists cycle in test spec.
    * @return true or false.
    */
    inline bool validate_cycle() {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        graph<T> test_case_graph(d_test_specs.size());
        for (auto it = d_test_specs.begin(); it != d_test_specs.end(); ++it)
        {
            if (it->get()->get_ptr_after_test_case() != nullptr)
            {
                test_case_graph.add_edge(get_index_of_test_case((*it)->get_ptr_after_test_case()),
                    get_index_of_test_case((*it)->get_ptr_test_case()));
            }
            if ((*it)->get_ptr_after_success_of_test_case() != nullptr)
            {
                test_case_graph.add_edge(get_index_of_test_case((*it)->get_ptr_after_success_of_test_case()),
                    get_index_of_test_case((*it)->get_ptr_test_case()));
            }
        }
        return test_case_graph.is_cyclic();
    }
    /**
    * This function checks test timeouts.
    * @return true or false.
    */
     void check_test_timeouts() {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        std::for_each(d_test_specs.begin(), d_test_specs.end(), [&](std::shared_ptr<test_run_spec<T>> spec) {
            if (spec->is_timeout())
                spec->complete(test_outcome::fail);
        });
    }
    /**
    * This function checks is all tests (sync & async) are completed.
    * @return true or false.
    */
    inline bool are_all_tests_completed() {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        return std::find_if(d_test_specs.begin(), d_test_specs.end(), [&](std::shared_ptr<test_run_spec<T>> spec) {
            return spec->state() != test_run_state::done;
        }) == d_test_specs.end();
    }
    /**
    * This function returns result of test cases.
    * @return collection of result.
    */
    inline std::vector<test_result> get_results() {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        return d_results;
    }
    /**
    * This function validates if a schedule is invalid.
    */
    inline void schedule_before(typename test_helper<T>::pmf_t before, typename test_helper<T>::pmf_t after) {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        if (internal_test_case_exists(before) && internal_test_case_exists(after)
            && get_index_of_test_case(before) > get_index_of_test_case(after))
        {
            auto after_it = d_test_specs.begin();
            std::advance(after_it, get_index_of_test_case(after));
            auto before_it = d_test_specs.begin();
            std::advance(before_it, get_index_of_test_case(before));
            throw std::invalid_argument("Test case " + (*before_it)->name() + " should be scheduled before " + (*after_it)->name() + ".");
        }
    }

    void complete_test_case(const std::string& name, test_outcome outcome)
    {
        instance(name).complete(outcome);
    }

    void add_thread_to_cancel(std::shared_ptr<tthread::thread> thread)
    {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        d_threads_to_cancel.push_back(thread);
    }

protected:
    virtual void before() {};
    virtual void after() {};
    virtual void setup_test_track() {};
    /**
    * Creates a clone of test run instance.
    * @param name Name of cloned test run instance.
    * @return Handle of cloned test run instance.
    */
    inline test_run_spec<T>& clone(const test_run_spec<T>& instance, const std::string& name) {
        d_logger.log(log_level::VERBOSE, "Cloning test case " + name + " for execution");
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        d_test_specs.push_back(instance.clone(name));
        test_run_spec<T>& new_spec = *d_test_specs.back();
        return new_spec;
    }

    /**
    * Continues test case execution asynchronously.
    * @param test_case_name Name of the test case.
    * @param async_func_obj Functional object to run asynchronously.
    * @param timeout Timeout of the functional object.
    */
    void go_async(const std::string& test_case_name
        , typename test_helper<T>::pmf_t func_obj)
    {
        assert(func_obj != nullptr);
        if (func_obj == nullptr)
            return;
        internal_async(test_case_name, func_obj, chrono::milliseconds(0));
    }
    /**
    * Continues test case execution asynchronously as a timer.
    * @param test_case_name Name of the test case.
    * @param timer_func_obj Functional object to call periodically.
    * @param interval Timer interval.
    * @param timeout Timeout of the general further timer working.
    */
    void start_timer(const std::string& test_case_name
        , typename test_helper<T>::pmf_t func_obj
        , const chrono::milliseconds& interval)
    {
        assert(func_obj != nullptr);
        if (func_obj == nullptr)
            return;
        assert(interval.count() > 0);
        if (interval.count() <= 0)
            return;
        internal_async(test_case_name, func_obj, interval);
    }
    /**
    * Stops the timer.
    * @param test_case_name Name of the test case.
    * @param timer_func_obj Timer functional object
    */
    void stop_timer(const std::string& test_case_name, typename test_helper<T>::pmf_t timer_func_obj)
    {
        instance(test_case_name).stop_timer(timer_func_obj);
    }
private:
    inline std::string start(std::string report_file_name) {
        d_logger.log(log_level::SILENT, "Starting execution of test suite : " + get_suite_name());
        // Execute tests
        while(true)
        {
            check_test_timeouts();
            auto test_case_it = get_test_case_to_execute();
            if (test_case_it != d_test_specs.end())
            {
                std::shared_ptr<test_run_spec<T>> test_case = *test_case_it;
                before();
                if (test_case->execute())
                    after();
            }
            else if (are_all_tests_completed())
                break;
            else
                tthread::this_thread::sleep_for(tthread::chrono::milliseconds(100));
        }
        extract_result();
        for (auto result = d_results.begin(); result != d_results.end(); ++result)
        {
            d_logger.log(log_level::REGULAR, "Outcome of " + result->test_case_name() + " is " + test_outcome_str[(int)result->outcome()] + ".");
        }
        d_logger.log(log_level::SILENT, "Done executing test suite : " + get_suite_name());
        return generate_junit_report(report_file_name);
    }
    typename std::list<std::shared_ptr<test_run_spec<T>>>::iterator get_test_case_to_execute() {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        return std::find_if(d_test_specs.begin(), d_test_specs.end(), [&](std::shared_ptr<test_run_spec<T>> spec) {
            return spec->state()==test_run_state::not_yet && spec->valid_to_execute();
        });
    }
    inline std::string generate_junit_report(std::string report_file_name) {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        testsuite_data test_suite(d_test_specs.size());
        for (auto it = d_test_specs.begin(); it != d_test_specs.end(); ++it)
        {
            test_run_spec<T>* test_spec = it->get();
            auto test_case = test_suite.add_testcase(get_suite_name() + "." + test_spec->name(), test_spec->name());
            if (test_spec->outcome() != test_outcome::pass)
            {
                test_case->fail_test_case(test_outcome_str[(int)test_spec->outcome()], "This has failed due to unknow reasons.");
            }
        }
        return test_suite.to_xml(report_file_name);
    }
    typename std::shared_ptr<test_run_spec<T>> schedule(
            std::function<void(const std::string&)> test_action,
            const std::string& test_case_name,
            typename test_helper<T>::pmf_t ptr_test_case,
            const chrono::milliseconds& timeout) {
        auto insert = true;
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        if (std::find_if(d_test_specs.begin(), d_test_specs.end(), [&](std::shared_ptr<test_run_spec<T>> spec) {
            return spec->get_ptr_test_case() == ptr_test_case && spec->name() == test_case_name;
        }) != d_test_specs.end())
        {
            insert = false;
        }
        if (insert)
        {
            d_test_specs.push_back(std::make_shared<test_run_spec<T>>(test_action,
                test_case_name, ptr_test_case, this, correct_timeout(timeout)));
        }
        for (auto it = d_test_specs.begin(); it != d_test_specs.end(); ++it)
        {
            auto ptr = *it;
            if (ptr->name() == test_case_name)
            {
                return ptr;
            }
        }
        return nullptr;
    }
    void extract_result() {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        d_results.clear();
        for (auto it = d_test_specs.begin(); it != d_test_specs.end(); ++it)
        {
            d_results.push_back(test_result((*it)->name(), (*it)->outcome()));
        }
    }
    int get_index_of_test_case(typename test_helper<T>::pmf_t test_case) {
        auto index = 0;
        for (auto it = d_test_specs.begin(); it != d_test_specs.end(); ++it, ++index)
        {
            if ((*it)->get_ptr_test_case() == test_case)
            {
                return index;
            }
        }
        return -1;
    }
    chrono::milliseconds correct_timeout(const chrono::milliseconds& in)
    {
        chrono::milliseconds timeout = in;
        if (timeout.count() == 0)
            timeout = d_default_timeout;
        if (timeout.count() > G2FASTH_MAX_TIMEOUT)
            timeout = chrono::milliseconds(G2FASTH_MAX_TIMEOUT);
        return timeout;
    }
    bool internal_test_case_exists(typename test_helper<T>::pmf_t test_case) {
        return std::find_if(d_test_specs.begin(), d_test_specs.end(), [&](std::shared_ptr<test_run_spec<T>> spec) {
            return spec->get_ptr_test_case() == test_case;
        }) != d_test_specs.end();
    }
    void internal_async(const std::string& test_case_name
        , typename test_helper<T>::pmf_t func_obj
        , const chrono::milliseconds& interval)
    {
        std::unique_ptr<async_run_data<T>> data(new async_run_data<T>);
        data->test_suite = this;
        data->test_case = nullptr;
        data->test_case_name = test_case_name;
        data->func_obj = std::bind(func_obj, static_cast<T*>(this), std::placeholders::_1);
        data->user_func_ptr = func_obj;
        data->interval = (int)interval.count();
        std::shared_ptr<tthread::thread> thread = std::make_shared<tthread::thread>(s_async_thread_proc, data.release());
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        d_threads_to_wait.push_back(thread);
    }
    static void s_async_thread_proc(void* p)
    {
        std::unique_ptr<async_run_data<T>> data((async_run_data<T>*)p);
        try {
            suite* _this = data->test_suite;
            test_run_spec<T>& test = _this->instance(data->test_case_name);
            if (test.state() != test_run_state::ongoing)
            {
                test.complete(test_outcome::fail);
                return;
            }
            try {
                if (test.execute(data.release()))
                    _this->after();
            }
            catch (... ) {
                test.complete(test_outcome::fail);
            }
        }
        catch (... ) {
        }
    }

private:
    tthread::mutex d_mutex;
    test_order d_order;
    std::list<std::shared_ptr<test_run_spec<T>>> d_test_specs;
    logger d_logger;
    std::vector<test_result> d_results;
    chrono::milliseconds d_default_timeout;
    std::list<std::shared_ptr<tthread::thread>> d_threads_to_wait;
    std::list<std::shared_ptr<tthread::thread>> d_threads_to_cancel;
};

}
}

#endif // !INC_LIBG2FASTH_SUITE_H