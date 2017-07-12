#ifndef INC_BASETEST_MYSUITE_H
#define INC_BASETEST_MYSUITE_H

#pragma once
#include "suite.hpp"

//using namespace std;
using namespace g2::fasth;

class MySuite : public g2::fasth::suite<MySuite>
{
public:
    MySuite(std::string, int, std::string, bool, std::string);
    ~MySuite();
    void first_test(const std::string&, test_run_reason);
    void second_test(const std::string&, test_run_reason);
    void third_test(const std::string&, test_run_reason);
    void fourth_test(const std::string&, test_run_reason);
    void timeout_pass_test(const std::string&, test_run_reason);
    void timeout_fail_test(const std::string&, test_run_reason);
    void default_timeout_pass_test(const std::string&, test_run_reason);
    void default_timeout_fail_test(const std::string&, test_run_reason);

    void sync_test(const std::string&, test_run_reason);
    void async_test_controlled(const std::string&, test_run_reason);
    void async_test_func_obj(const std::string&, test_run_reason);
    void async_test_uncontrolled(const std::string&, test_run_reason);

    void timer_test(const std::string&, test_run_reason);     // Main test method
    void timer_func_obj(const std::string&, test_run_reason); // Timer functional object
    void timer_monitor(const std::string&, test_run_reason);  // Some async helper just for demo

private:
    const int d_iParam;
    const std::string d_sParam;
    const bool d_bParam;
    void before() override;
    void after() override;
    void setup_test_track() override;
    std::list<std::shared_ptr<tthread::thread>> d_threads;
};

#endif // !INC_BASETEST_MYSUITE_H