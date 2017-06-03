#ifndef INC_BASETEST_MYSUITE_H
#define INC_BASETEST_MYSUITE_H

#pragma once
#include "suite.hpp"

class MySuite : public g2::fasth::suite<MySuite>
{
public:
    MySuite(std::string, int, std::string, bool, std::string);
    g2::fasth::test_outcome first_test(g2::fasth::test_run_instance &);
    g2::fasth::test_outcome second_test(g2::fasth::test_run_instance &);
    g2::fasth::test_outcome third_test(g2::fasth::test_run_instance &);
    g2::fasth::test_outcome fourth_test(g2::fasth::test_run_instance &);
    g2::fasth::test_outcome timeout_pass_test(g2::fasth::test_run_instance &);
    g2::fasth::test_outcome timeout_fail_test(g2::fasth::test_run_instance &);
    g2::fasth::test_outcome default_timeout_pass_test(g2::fasth::test_run_instance &);
    g2::fasth::test_outcome default_timeout_fail_test(g2::fasth::test_run_instance &);
private:
    const int d_iParam;
    const std::string d_sParam;
    const bool d_bParam;
    void before() override;
    void after() override;
    void setup_test_track() override;
    std::vector<std::shared_ptr<tthread::thread>> d_threads;
};

#endif // !INC_BASETEST_MYSUITE_H