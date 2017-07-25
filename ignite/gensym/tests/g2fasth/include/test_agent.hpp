#pragma once
#ifndef INC_LIBG2FASTH_TEST_AGENT_H
#define INC_LIBG2FASTH_TEST_AGENT_H

#include <list>
#include <algorithm>
#include <assert.h>
#include "base_suite.hpp"
#include "tinythread.h"
#include <ctime>

namespace g2 {
namespace fasth {
/**
* This class store set of suites that can be executed in a specific order.
* All suites can be executed via single instruction.
*/
class test_agent {
    struct suite_pair
    {
        std::shared_ptr<base_suite> first, second;
        bool background;
        suite_pair(std::shared_ptr<base_suite> f, std::shared_ptr<base_suite> s, bool bg): first(f), second(s), background(bg) {}
    };
public:
    test_agent(test_order order=test_order::random): d_order(order)
        , d_sleep_quantum(100)
    {
        d_max_concurrency = tthread::thread::hardware_concurrency();
        if (d_max_concurrency < 16)
            d_max_concurrency = 16;
        d_thread_pool = std::make_shared<tthread::thread_pool>();
    }
    ~test_agent()
    {
    }
    /**
    * This function accepts a pointer to test suite and schedule it for sequential execution.
    * Pointer is used as its list is to be maintained.
    * Naked pointer is used for easier syntax of its usage.
    * @param suite_to_run pointer to test suite.
    * @param suite_after pointer to test suite after which the scheduled suite will start.
    */
    inline void schedule_suite(std::shared_ptr<base_suite> suite_to_run
        , std::shared_ptr<base_suite> suite_after=nullptr)
    {
        internal_schedule_suite(suite_to_run, suite_after, false);
    }
    /**
    * This function accepts a pointer to test suite and schedule it for background execution.
    * Pointer is used as its list is to be maintained.
    * Naked pointer is used for easier syntax of its usage.
    * @param suite_to_run pointer to test suite.
    * @param suite_after pointer to test suite after which the scheduled suite will start.
    */
    inline void schedule_background_suite(std::shared_ptr<base_suite> suite_to_run
        , std::shared_ptr<base_suite> suite_after=nullptr)
    {
        internal_schedule_suite(suite_to_run, suite_after, true);
    }
    /**
    * This function executes all test suites in its queue.
    * @param parallel Flag that specifies how to run suites: sequentially or concurrently.
    */
    inline void execute() {
        auto bg_count = std::count_if(d_suites.begin(), d_suites.end(), [&](suite_pair sp) { return sp.background; });
        std::list<unsigned> task_ids;
        for (unsigned i=0; i<bg_count; i++)
        {   // Start background suites
            task_ids.push_back(d_thread_pool->add_task(s_thread_proc, this));
        }
        if (d_suites.size()-bg_count > 0)
            internal_execute(false); // Run sequential suites
        while (task_ids.size())
        {
            d_thread_pool->join_task(task_ids.front());
            task_ids.pop_front();
        }
    }
    void set_concurrency(unsigned new_concurrency)
    {
        if (new_concurrency > d_max_concurrency)
            throw std::invalid_argument("Concurrency could not be more than " + std::to_string((long long)d_max_concurrency));
        d_thread_pool->set_thread_limit(new_concurrency);
    }
    void set_sleep_quantum(const tthread::chrono::milliseconds& new_quantum)
    {
        if (new_quantum.count() > 1000)
            throw std::invalid_argument("Sleep quantum could not be more than 1 second");
        if (new_quantum.count() < 1)
            throw std::invalid_argument("Sleep quantum could not be less than 1 millisecond");
        d_sleep_quantum = new_quantum;
    }
private:
    inline void internal_schedule_suite(std::shared_ptr<base_suite> suite_to_run
        , std::shared_ptr<base_suite> suite_after, bool background)
    {
        if (suite_to_run == nullptr)
            throw std::invalid_argument("Could not schedule nullptr");
        if (is_suite_scheduled(suite_to_run))
            throw std::invalid_argument("Could not schedule the suite " + suite_to_run->get_suite_name() + " again.");
        if (suite_after && !is_suite_scheduled(suite_after))
            throw std::invalid_argument("Could not schedule the suite " + suite_to_run->get_suite_name() + " to run after a suite that is not scheduled.");
        d_suites.push_back(suite_pair(suite_to_run,suite_after,background));
    }
    bool is_suite_scheduled(std::shared_ptr<base_suite> suite_to_run) {
        return std::find_if(d_suites.begin(), d_suites.end(), [&](suite_pair sp) {
            return sp.first->get_suite_name() == suite_to_run->get_suite_name();
        }) != d_suites.end();
    }
    static void s_thread_proc(void* p)
    {
        try {
            ((test_agent*)p)->internal_execute(true);
        } catch (...) {}
    }
    void internal_execute(bool background)
    {
        while (true)
        {
            std::shared_ptr<base_suite> suite;
            while (suite = get_suite_to_run(background))
            {
                if (suite->is_parallel())
                    d_thread_pool->increase_thread_limit();
                suite->execute(d_thread_pool);
                suite->set_state(done);
            }
            if (are_all_suites_completed(background))
                break;
            tthread::this_thread::sleep_for(d_sleep_quantum);
        }
    }
    std::shared_ptr<base_suite> get_suite_to_run(bool background)
    {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        std::list<suite_pair> suites;
        std::for_each(d_suites.begin(), d_suites.end(), [&](suite_pair sp) {
            if (sp.background==background && sp.first->state()==not_yet && (!sp.second || sp.second->state()==done))
                suites.push_back(sp);
        });
        if (0 == suites.size())
            return nullptr;
        std::shared_ptr<base_suite> suite = suites.front().first;
        if (test_order::random==d_order && suites.size()>1)
        {
            srand(time(NULL));
            int rnd = rand() % suites.size();
            auto it = suites.begin();
            for (int i=0; i<rnd; i++)
                it++;
            suite = it->first;
        }
        suite->set_state(ongoing);
        return suite;
    }
    bool are_all_suites_completed(bool background)
    {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        return std::find_if(d_suites.begin(), d_suites.end(), [&](suite_pair sp) {
            return sp.background==background && sp.first->state() != done;
        }) == d_suites.end();
    }
private:
    tthread::mutex d_mutex;
    std::list<suite_pair> d_suites;
    unsigned d_max_concurrency;
    test_order d_order;
    tthread::chrono::milliseconds d_sleep_quantum;
    std::shared_ptr<tthread::thread_pool> d_thread_pool;
};
}
}
#endif // !INC_LIBG2FASTH_TEST_AGENT_H