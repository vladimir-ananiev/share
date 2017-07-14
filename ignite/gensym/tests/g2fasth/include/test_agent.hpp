#pragma once
#ifndef INC_LIBG2FASTH_TEST_AGENT_H
#define INC_LIBG2FASTH_TEST_AGENT_H

#include <list>
#include <algorithm>
#include <assert.h>
#include "base_suite.hpp"
#include "tinythread.h"

namespace g2 {
namespace fasth {
/**
* This class store set of suites that can be executed in a specific order.
* All suites can be executed via single instruction.
*/
class test_agent {
    typedef std::pair<std::shared_ptr<base_suite>,std::shared_ptr<base_suite>> suite_pair;
public:
    test_agent(): d_concurrency(tthread::thread::hardware_concurrency())
    {
        d_concurrency = tthread::thread::hardware_concurrency();
        d_max_concurrency = d_concurrency * 4;
        if (d_max_concurrency < 16)
            d_max_concurrency = 16;
    }
    ~test_agent()
    {
    }
    /**
    * This function accepts a pointer to test suite and schedule it.
    * Pointer is used as its list is to be maintained.
    * Naked pointer is used for easier syntax of its usage.
    * @param suite_to_run pointer to test suite.
    * @return instance of test agent.
    */
    inline void schedule_suite(std::shared_ptr<base_suite> suite_to_run
        , std::shared_ptr<base_suite> suite_after=nullptr)
    {
        if (suite_to_run == nullptr)
            throw std::invalid_argument("Could not schedule nullptr");
        if (is_suite_scheduled(suite_to_run))
            throw std::invalid_argument("Could not schedule the suite " + suite_to_run->get_suite_name() + " again.");
        if (suite_after && !is_suite_scheduled(suite_after))
            throw std::invalid_argument("Could not schedule the suite " + suite_to_run->get_suite_name() + " to run after a suite that is not scheduled.");
        d_suites.push_back(suite_pair(suite_to_run,suite_after));
    }
    /**
    * This function executes all test suites in its queue.
    */
    inline void execute(bool parallel=false) {
        int concurrency = d_concurrency;
        if (d_suites.size() < concurrency)
            concurrency = d_suites.size();
        if (concurrency < 2)
            parallel = false;
        if (!parallel)
            return internal_execute();
        std::list<std::shared_ptr<tthread::thread>> threads;
        for (unsigned i=0; i<concurrency; i++)
        {
            threads.push_back(std::make_shared<tthread::thread>(s_thread_proc, this));
        }
        while (threads.size())
        {
            threads.front()->join();
            threads.pop_front();
        }
    }
    void set_concurrency(unsigned new_concurrency)
    {
        if (new_concurrency > d_max_concurrency)
            throw std::invalid_argument("Concurrency could not be more than " + std::to_string((long long)d_max_concurrency));
        d_concurrency = new_concurrency;
    }
private:
    tthread::mutex d_mutex;
    std::list<suite_pair> d_suites;
    unsigned d_concurrency;
    unsigned d_max_concurrency;
    bool is_suite_scheduled(std::shared_ptr<base_suite> suite_to_run) {
        return std::find_if(d_suites.begin(), d_suites.end(), [&](suite_pair sp) {
            return sp.first->get_suite_name() == suite_to_run->get_suite_name();
        }) != d_suites.end();
    }
    static void s_thread_proc(void* p)
    {
        try {
            ((test_agent*)p)->internal_execute();
        } catch (...) {}
    }
    void internal_execute()
    {
        while (true)
        {
            std::shared_ptr<base_suite> suite;
            while (suite = get_suite_to_run())
            {
                suite->execute();
                suite->set_state(done);
            }
            if (are_all_suites_completed())
                break;
            tthread::this_thread::sleep_for(tthread::chrono::milliseconds(10));
        }
    }
    std::shared_ptr<base_suite> get_suite_to_run()
    {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        auto it = std::find_if(d_suites.begin(), d_suites.end(), [&](suite_pair sp) {
            return sp.first->state()==not_yet && (!sp.second || sp.second->state()==done);
        });
        if (d_suites.end() == it)
            return nullptr;
        it->first->set_state(ongoing);
        return it->first;
    }
    bool are_all_suites_completed()
    {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        return std::find_if(d_suites.begin(), d_suites.end(), [&](suite_pair sp) {
            return sp.first->state() != done;
        }) == d_suites.end();
    }
};
}
}
#endif // !INC_LIBG2FASTH_TEST_AGENT_H