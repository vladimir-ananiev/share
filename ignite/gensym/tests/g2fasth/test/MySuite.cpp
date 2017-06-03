#include <cstdio>
#include "MySuite.hpp"
#include "tinythread.h"
#include "libgsi.hpp"

using namespace std;
using namespace g2::fasth;

void test_case_thread(void * aArg)
{
    test_run_instance * test_instance = static_cast<test_run_instance*>(aArg);
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(1000));
    test_instance->set_outcome(test_outcome::pass);
}

MySuite::MySuite(string suite_name, int iParam, std::string sParam, bool bParam, std::string logger_path)
    : suite(suite_name
        , test_order::implied
        , g2::fasth::log_level::VERBOSE
        , logger_path
        , chrono::milliseconds(5000)) // default timeout
    , d_iParam(iParam)
    , d_sParam(sParam)
    , d_bParam(bParam)
{
}

void MySuite::before()
{
    get_logger().log(g2::fasth::log_level::VERBOSE, "Calling before().");
}

void MySuite::after()
{
    get_logger().log(g2::fasth::log_level::VERBOSE, "Calling after().");
}

void MySuite::setup_test_track()
{
    //auto& first = run(&MySuite::first_test, "TestA");
    ////run(&MySuite::second_test, "TestB").after(first);
    //run(&MySuite::second_test, "TestB").after(&MySuite::first_test);
    //run(&MySuite::third_test, "TestC").after_success_of(instance("TestB"));
    //run(&MySuite::third_test, "TestC").guard([this] { return this->d_iParam > 0; });
    //run(&MySuite::fourth_test, "AsyncTest");
    //clone(first, "TestA Again");

    //g2::fasth::libgsi::getInstance().declare_g2_variable<int>("INTEGER_VAR_NAME");

    //run(&MySuite::timeout_pass_test, "Timeout-Pass-Test", chrono::milliseconds(4000)); // timeout 4000 ms
    run(&MySuite::timeout_fail_test, "Timeout-Fail-Test", chrono::milliseconds(2000)); // timeout 2000 ms
    //run(&MySuite::default_timeout_pass_test, "Default-Timeout-Pass-Test");    // default timeout
    //run(&MySuite::default_timeout_fail_test, "Default-Timeout-Fail-Test");    // default timeout
}

test_outcome MySuite::first_test(g2::fasth::test_run_instance &)
{
    char buf[25];
    sprintf(buf, "%d", d_iParam);
    get_logger().log(g2::fasth::log_level::VERBOSE, std::string("Value of d_sParam is ") + buf);
    get_logger().log(g2::fasth::log_level::VERBOSE, std::string("Value of d_sParam is ") + d_sParam);
    get_logger().log(g2::fasth::log_level::VERBOSE, std::string("Value of d_bParam is ") + (d_bParam ? "true" : "false"));
    return test_outcome::pass;
}

test_outcome MySuite::second_test(test_run_instance & test_instance)
{
    return test_outcome::pass;
}

test_outcome MySuite::third_test(g2::fasth::test_run_instance &)
{
    return test_outcome::pass;
}

test_outcome MySuite::fourth_test(g2::fasth::test_run_instance & test_instance)
{
    std::shared_ptr<tthread::thread> ptr = std::make_shared<tthread::thread>(test_case_thread, &test_instance);
    d_threads.push_back(ptr);
    return test_outcome::by_instance;
}

test_outcome MySuite::timeout_pass_test(g2::fasth::test_run_instance &)
{
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(3000));
    return test_outcome::pass;
}

test_outcome MySuite::timeout_fail_test(g2::fasth::test_run_instance &)
{
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(3000));
    return test_outcome::pass;
}

test_outcome MySuite::default_timeout_pass_test(g2::fasth::test_run_instance &)
{
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(4000));
    return test_outcome::pass;
}

test_outcome MySuite::default_timeout_fail_test(g2::fasth::test_run_instance &)
{
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(6000));
    return test_outcome::pass;
}
