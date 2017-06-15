#pragma once
#ifndef INC_LIBG2FASTH_SUITE_H
#define INC_LIBG2FASTH_SUITE_H

#include <list>
#include <vector>
#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>
#include <iterator>
#include <assert.h>
#include <fstream>
#include <ios>
#include "g2fasth_enums.hpp"
#include "test_run_spec.hpp"
#include "logger.hpp"
#include "g2fasth_typedefs.hpp"
#include "junit_report.hpp"
#include "base_suite.hpp"
#include "tinythread.h"

namespace g2 {
namespace fasth {

/**
* Stores relationship between a test case and its outcome.
*/
struct test_result {
    test_result(std::string test_case_name, g2::fasth::test_outcome test_outcome)
        : d_test_case_name(test_case_name), d_test_outcome(test_outcome)
    {
    }
    std::string test_case_name() {
        return d_test_case_name;
    }
    g2::fasth::test_outcome test_outcome() {
        return d_test_outcome;
    }
private:
    std::string d_test_case_name;
    g2::fasth::test_outcome d_test_outcome;
};

// Max timeout for tests
#define G2FASTH_MAX_TIMEOUT      7*24*3600*1000  // 1 week
// Default timeout for tests
#define G2FASTH_DEFAULT_TIMEOUT  600*1000        // 10 min

template <class T>
class suite : public g2::fasth::base_suite {
public:
    /**
    * Constructor that sets the name of the suite, its execution order and log level.
    * @param suite_name name of the test suite.
    * @param test_order order of execution of test cases.
    * @param logger_file_path absolute path to the log file.
    * @param suite_log_level log level of test suite.
    */
    suite(std::string suite_name, g2::fasth::test_order test_order,
        g2::fasth::log_level suite_log_level, std::string logger_file_path = "", 
        const chrono::milliseconds& default_timeout=chrono::milliseconds(G2FASTH_DEFAULT_TIMEOUT))
        : base_suite(suite_name)
        , d_order(test_order)
        , d_logger(g2::fasth::logger(suite_log_level))
        , d_default_timeout(default_timeout)
    {
        if (d_default_timeout.count() > G2FASTH_MAX_TIMEOUT)
            d_default_timeout = chrono::milliseconds(G2FASTH_MAX_TIMEOUT);
        d_logger.add_output_stream(std::cout, g2::fasth::log_level::REGULAR);
        if (!logger_file_path.empty())
        {
            d_logger.add_output_stream(new std::ofstream(logger_file_path, std::ios_base::app), g2::fasth::log_level::VERBOSE);
        }
    }
    /**
    * This function executes test suite. First it setups test track and then starts execution.
    * @param report_file_name Absolute path of the JUnit report xml, can be empty if report is not to be written
    * @return JUnit report in string format.
    */
    inline std::string execute(std::string report_file_name = "") override {
        if (d_test_specs.size() == 0)
        {
            d_logger.log(g2::fasth::log_level::SILENT, "Setting up test track for test suite : " + get_suite_name());
            setup_test_track();
            d_logger.log(g2::fasth::log_level::SILENT, "Test track setup done for test suite : " + get_suite_name());
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
    inline g2::fasth::test_run_spec<T>& run(typename g2::fasth::test_helper<T>::pmf_t ptr_test_case
            , std::string test_case_name, const chrono::milliseconds& timeout=chrono::milliseconds(0)) {
        auto iterator = schedule(std::bind(ptr_test_case, static_cast<T*>(this), std::placeholders::_1), test_case_name, ptr_test_case, timeout);
        assert(iterator != d_test_specs.end());
        return *iterator;
    }
    /**
    * This function returns true or false depending upon condition if any test case is scheduled to execute.
    * @return A boolean value.
    */
    inline bool test_case_exists() {
        return std::find_if(d_test_specs.begin(), d_test_specs.end(), [&](g2::fasth::test_run_spec<T> spec) {
            return spec.outcome() == g2::fasth::test_outcome::indeterminate;
        }) != d_test_specs.end();
    }
    /**
    * Returns test run instance by name.
    * @param name Name of test run instance to return.
    * @return Handle of specified test run instance.
    */
    inline test_run_spec<T>& instance(std::string name) {
        auto parent = std::find_if(d_test_specs.begin(), d_test_specs.end(), [&](test_run_spec<T> spec) {
            return spec.name() == name;
        });
        if (parent == d_test_specs.end())
        {
            throw std::invalid_argument("Instance " + name + " doesn't exist.");
        }
        return *parent;
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
    inline bool test_case_exists(typename g2::fasth::test_helper<T>::pmf_t test_case) {
        return std::find_if(d_test_specs.begin(), d_test_specs.end(), [&](g2::fasth::test_run_spec<T> spec) {
            return spec.get_ptr_test_case() == test_case;
        }) != d_test_specs.end();
    }
    /**
    * This function returns instance of spec.
    * @param ptr_test_case pointer to test case for which test spec is to be returned.
    * @return instance of test spec.
    */
    inline g2::fasth::test_run_spec<T> get_spec(typename g2::fasth::test_helper<T>::pmf_t ptr_test_case) {
        std::vector<g2::fasth::test_run_spec<T>> matches;
        std::copy_if(d_test_specs.begin(), d_test_specs.end(), std::back_inserter(matches),
            [&](g2::fasth::test_run_spec<T> spec) {
            return spec.get_ptr_test_case() == ptr_test_case;
        });
        return matches[0];
    }
    /**
    * This function validates if there exists cycle in test spec.
    * @return true or false.
    */
    inline bool validate_cycle() {
        g2::fasth::graph<T> test_case_graph(d_test_specs.size());
        for (auto ptr = d_test_specs.begin(); ptr != d_test_specs.end(); ++ptr)
        {
            if (ptr->get_ptr_after_test_case() != nullptr)
            {
                test_case_graph.add_edge(get_index_of_test_case(ptr->get_ptr_after_test_case()),
                    get_index_of_test_case(ptr->get_ptr_test_case()));
            }
            if (ptr->get_ptr_after_success_of_test_case() != nullptr)
            {
                test_case_graph.add_edge(get_index_of_test_case(ptr->get_ptr_after_success_of_test_case()),
                    get_index_of_test_case(ptr->get_ptr_test_case()));
            }
        }
        return test_case_graph.is_cyclic();
    }
    /**
    * This function validates if a test is pending to run or alread running.
    * @return true or false.
    */
    inline bool test_case_running_or_pending() {
        return std::find_if(d_test_specs.begin(), d_test_specs.end(), [&](g2::fasth::test_run_spec<T> spec) {
            return spec.state() == g2::fasth::test_run_state::not_yet || spec.state() == g2::fasth::test_run_state::ongoing;
        }) != d_test_specs.end();
    }
    /**
    * This function returns result of test cases.
    * @return collection of result.
    */
    inline std::vector<g2::fasth::test_result> get_results() {
        return d_results;
    }
    /**
    * This function validates if a schedule is invalid.
    */
    inline void schedule_before(typename g2::fasth::test_helper<T>::pmf_t before, typename g2::fasth::test_helper<T>::pmf_t after) {
        if (test_case_exists(before)
            && test_case_exists(after)
            && get_index_of_test_case(before) > get_index_of_test_case(after))
        {
            auto after_it = d_test_specs.begin();
            std::advance(after_it, get_index_of_test_case(after));
            auto before_it = d_test_specs.begin();
            std::advance(before_it, get_index_of_test_case(before));
            throw std::invalid_argument("Test case " + (*before_it).name() + " should be scheduled before " + (*after_it).name() + ".");
        }
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
    inline test_run_spec<T>& clone(const test_run_spec<T>& instance, std::string name) {
        d_logger.log(g2::fasth::log_level::VERBOSE, "Cloning test case " + name + " for execution");
        d_test_specs.push_back(instance);
        test_run_spec<T>& new_spec = d_test_specs.back();
        new_spec.set_name(name);
        return new_spec;
    }
private:
    g2::fasth::test_order d_order;
    std::list<g2::fasth::test_run_spec<T>> d_test_specs;
    logger d_logger;
    std::vector<g2::fasth::test_result> d_results;
    chrono::milliseconds d_default_timeout;
    inline std::string start(std::string report_file_name) {
        d_logger.log(g2::fasth::log_level::SILENT, "Starting execution of test suite : " + get_suite_name());
        while (test_case_running_or_pending())
        {
            auto pending_test_case = get_pending_test_case();
            while (pending_test_case != d_test_specs.end())
            {
                if (pending_test_case->valid_to_execute())
                {
                    before();
                    auto is_execution_async = pending_test_case->execute();
                    if (!is_execution_async)
                    {
                        after();
                    }
                }
                update_async_tests();
                pending_test_case = get_pending_test_case();
            }
            update_async_tests();
        }
        extract_result();
        for (auto result = d_results.begin(); result != d_results.end(); ++result)
        {
            d_logger.log(g2::fasth::log_level::REGULAR, "Outcome of " + result->test_case_name() + " is " + test_outcome_str[(int)result->test_outcome()] + ".");
        }
        d_logger.log(g2::fasth::log_level::SILENT, "Done executing test suite : " + get_suite_name());
        return generate_junit_report(report_file_name);
    }
    typename std::list<g2::fasth::test_run_spec<T>>::iterator get_pending_test_case() {
        return std::find_if(d_test_specs.begin(), d_test_specs.end(), [&](g2::fasth::test_run_spec<T> spec) {
            return spec.state() == g2::fasth::test_run_state::not_yet;
        });
    }
    inline std::string generate_junit_report(std::string report_file_name) {
        g2::fasth::testsuite_data test_suite(d_test_specs.size());
        for (auto test_spec = d_test_specs.begin(); test_spec != d_test_specs.end(); ++test_spec)
        {
            auto test_case = test_suite.add_testcase(get_suite_name() + "." + test_spec->name(), test_spec->name());
            if (test_spec->outcome() != test_outcome::pass)
            {
                test_case->fail_test_case(test_outcome_str[(int)test_spec->outcome()], "This has failed due to unknow reasons.");
            }
        }
        return test_suite.to_xml(report_file_name);
    }
    typename std::list<g2::fasth::test_run_spec<T>>::iterator schedule(
            std::function<g2::fasth::test_outcome(g2::fasth::test_run_instance &)> test_case,
            std::string test_case_name,
            typename g2::fasth::test_helper<T>::pmf_t ptr_test_case,
             chrono::milliseconds timeout) {
        auto insert = true;
        if (std::find_if(d_test_specs.begin(), d_test_specs.end(), [&](g2::fasth::test_run_spec<T> spec) {
            return spec.get_ptr_test_case() == ptr_test_case && spec.name() == test_case_name;
        }) != d_test_specs.end())
        {
            insert = false;
        }
        if (insert)
        {
            if (timeout.count() == 0)
                timeout = d_default_timeout;
            if (timeout.count() > G2FASTH_MAX_TIMEOUT)
                timeout = chrono::milliseconds(G2FASTH_MAX_TIMEOUT);
            d_test_specs.emplace_back(test_run_spec<T>(test_case, test_case_name, ptr_test_case, *this, timeout));
        }
        for (auto it = d_test_specs.begin(); it != d_test_specs.end(); ++it)
        {
            if (it->name() == test_case_name)
            {
                return it;
            }
        }
        return d_test_specs.end();
    }
    inline void execute_pending_test_case() {
        auto pending_test_case = std::find_if(d_test_specs.begin(), d_test_specs.end(), [&](g2::fasth::test_run_spec<T> spec) {
            return spec.outcome() == g2::fasth::test_outcome::indeterminate;
        });
        d_logger.log(g2::fasth::log_level::VERBOSE, "Starting execution of test case : " + pending_test_case->name());
        pending_test_case->execute();
        d_logger.log(g2::fasth::log_level::VERBOSE, "Done executing test case : " + pending_test_case->name());
    }
    inline void update_async_tests() {
        auto loop_counter = 0;
        auto max_loop_counter = 3;
        auto max_wait_seconds = 1;
        while (loop_counter < max_loop_counter)
        {
            auto running_test_case = std::find_if(d_test_specs.begin(), d_test_specs.end(), [&](g2::fasth::test_run_spec<T> spec) {
                return spec.outcome() == g2::fasth::test_outcome::by_instance;
            });
            if (running_test_case == d_test_specs.end())
            {
                break;
            }
            if (running_test_case->execution_outcome() != g2::fasth::test_outcome::indeterminate)
            {
                running_test_case->set_outcome(running_test_case->execution_outcome());
                running_test_case->set_state(g2::fasth::test_run_state::done);
                d_logger.log(g2::fasth::log_level::VERBOSE, "Executed test case : " + running_test_case->name());
                after();
            }
            loop_counter++;
            tthread::this_thread::sleep_for(tthread::chrono::milliseconds(max_wait_seconds * 1000));
        }
    }
    inline void extract_result() {
        d_results.clear();
        for (auto spec = d_test_specs.begin(); spec != d_test_specs.end(); ++spec)
        {
            d_results.push_back(g2::fasth::test_result(spec->name(), spec->outcome()));
        }
    }
    int get_index_of_test_case(typename g2::fasth::test_helper<T>::pmf_t test_case) {
        auto index = 0;
        for (auto it = d_test_specs.begin(); it != d_test_specs.end(); ++it, ++index)
        {
            if (it->get_ptr_test_case() == test_case)
            {
                return index;
            }
        }
        return -1;
    }
};

}
}

#endif // !INC_LIBG2FASTH_SUITE_H