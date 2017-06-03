#pragma once
#ifndef INC_LIBG2FASTH_TEST_AGENT_H
#define INC_LIBG2FASTH_TEST_AGENT_H

#include <list>
#include <assert.h>
#include "base_suite.hpp"

namespace g2 {
namespace fasth {
/**
* This class store set of suites that can be executed in a specific order.
* All suites can be executed via single instruction.
*/
class test_agent {
public:
    test_agent()
    {
    }
    ~test_agent()
    {
        d_last_scheduled_test_suite.reset();
    }
    /**
    * This function accepts a pointer to test suite and schedule it.
    * Pointer is used as its list is to be maintained.
    * Naked pointer is used for easier syntax of its usage.
    * @param suite_to_run pointer to test suite.
    * @return instance of test agent.
    */
    inline test_agent& run_suite(std::shared_ptr<g2::fasth::base_suite> suite_to_run) {
        if (suite_scheduled(suite_to_run))
        {
            throw std::invalid_argument("can't schedule " + suite_to_run->get_suite_name() + " again.");
        }
        d_last_scheduled_test_suite = suite_to_run;
        d_suites.push_back(suite_to_run);
        return *this;
    }
    /**
    * This function executes all test suites in its queue.
    */
    inline void execute() {
        for (auto suite = d_suites.begin(); suite != d_suites.end(); ++suite)
        {
            (*suite)->execute();
        }
    }
    /**
    * This function accepts a pointer to test suite and schedule it after the dependent test case.
    * Pointer is used as its list is to be maintained.
    * Naked pointer is used for easier syntax of its usage.
    * @param suite_to_run pointer to test suite.
    */
    inline void after(std::shared_ptr<g2::fasth::base_suite> suite_to_run) {
        auto pointer = d_last_scheduled_test_suite.lock();
        if (pointer == nullptr || pointer->get_suite_name().empty())
        {
            throw std::invalid_argument("after can only be called after scheduling a test suite.");
        }
        if (pointer->get_suite_name() == suite_to_run->get_suite_name())
        {
            throw std::invalid_argument("same suite cannot be scheduled as dependent suite.");
        }
        if (suite_scheduled(suite_to_run))
        {
            throw std::invalid_argument("can't schedule " + suite_to_run->get_suite_name() + " again.");
        }
        insert_test_suite_after(suite_to_run);
    }
private:
    std::list<std::shared_ptr<g2::fasth::base_suite>> d_suites;
    std::weak_ptr<g2::fasth::base_suite> d_last_scheduled_test_suite;
    inline bool suite_scheduled(std::shared_ptr<g2::fasth::base_suite> suite_to_run) {
        return std::find_if(d_suites.begin(), d_suites.end(), [&](std::shared_ptr<g2::fasth::base_suite> suite) {
            return suite->get_suite_name() == suite_to_run->get_suite_name();
        }) != d_suites.end();
    }
    inline void insert_test_suite_after(std::shared_ptr<g2::fasth::base_suite> suite_to_run) {
        auto pointer = d_last_scheduled_test_suite.lock();
        auto iterator = get_itr_of_test_suite(pointer->get_suite_name());
        assert(iterator != d_suites.end());
        d_suites.insert(iterator, suite_to_run);
    }
    inline std::list<std::shared_ptr<g2::fasth::base_suite>>::iterator get_itr_of_test_suite(std::string test_case_name) {
        for (auto it = d_suites.begin(); it != d_suites.end(); ++it)
        {
            if ((*it)->get_suite_name() == test_case_name)
            {
                return it;
            }
        }
        return d_suites.end();
    }
};
}
}
#endif // !INC_LIBG2FASTH_TEST_AGENT_H