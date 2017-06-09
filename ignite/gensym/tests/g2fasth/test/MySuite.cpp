#include <cstdio>
#include "MySuite.hpp"
#include "tinythread.h"
#include "libgsi.hpp"

using namespace std;
using namespace g2::fasth;

class ScopeLog
{
    std::string name;
public:
    ScopeLog(const std::string& name): name(name)
    {
        printf("%08u %s ENTER\n", GetTickCount(), this->name.c_str());
    }
    ~ScopeLog()
    {
        printf("%08u %s LEAVE\n", GetTickCount(), this->name.c_str());
    }
};
#define FUNCLOG ScopeLog __func_log__(__FUNCTION__)

struct thread_data
{
    MySuite* suite;
    std::string test_case_name;
};

void uncontrolled_test_case_thread(void* p);

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
MySuite::~MySuite()
{
    while (d_threads.size())
    {
        d_threads.front()->join();
        d_threads.pop_front();
    }
}


void MySuite::before()
{
    get_logger().log(g2::fasth::log_level::VERBOSE, "Calling before().");
}

void MySuite::after()
{
    get_logger().log(g2::fasth::log_level::VERBOSE, "Calling after().");
}

void MySuite::first_test(const std::string& test_case_name)
{
    char buf[25];
    sprintf(buf, "%d", d_iParam);
    get_logger().log(g2::fasth::log_level::VERBOSE, std::string("Value of d_sParam is ") + buf);
    get_logger().log(g2::fasth::log_level::VERBOSE, std::string("Value of d_sParam is ") + d_sParam);
    get_logger().log(g2::fasth::log_level::VERBOSE, std::string("Value of d_bParam is ") + (d_bParam ? "true" : "false"));
    
    complete_test_case(test_case_name, test_outcome::pass);
}

void MySuite::second_test(const std::string& test_case_name)
{
    complete_test_case(test_case_name, test_outcome::pass);
}

void MySuite::third_test(const std::string& test_case_name)
{
    complete_test_case(test_case_name, test_outcome::pass);
}

void MySuite::fourth_test(const std::string& test_case_name)
{
    thread_data* data = new thread_data;
    data->suite = this;
    data->test_case_name = test_case_name;

    std::shared_ptr<tthread::thread> ptr = std::make_shared<tthread::thread>(uncontrolled_test_case_thread, data);
    d_threads.push_back(ptr);
}

void MySuite::timeout_pass_test(const std::string& test_case_name)
{
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(3000));

    complete_test_case(test_case_name, test_outcome::pass);
}

void MySuite::timeout_fail_test(const std::string& test_case_name)
{
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(3000));

    complete_test_case(test_case_name, test_outcome::pass);
}

void MySuite::default_timeout_pass_test(const std::string& test_case_name)
{
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(4000));

    complete_test_case(test_case_name, test_outcome::pass);
}

void MySuite::default_timeout_fail_test(const std::string& test_case_name)
{
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(6000));

    complete_test_case(test_case_name, test_outcome::pass);
}

void MySuite::setup_test_track()
{
    auto& first = run(&MySuite::first_test, "TestA");
    //run(&MySuite::second_test, "TestB").after(first);
    run(&MySuite::second_test, "TestB").after(&MySuite::first_test);
    run(&MySuite::third_test, "TestC").after_success_of(instance("TestB"));
    run(&MySuite::third_test, "TestC").guard([this] { return this->d_iParam > 0; });
    run(&MySuite::fourth_test, "AsyncTest");
    clone(first, "TestA Again");

    g2::fasth::libgsi::getInstance().declare_g2_variable<int>("INTEGER_VAR_NAME");

    run(&MySuite::timeout_pass_test, "Timeout-Pass-Test", chrono::milliseconds(4000)); // timeout 4000 ms
    run(&MySuite::timeout_fail_test, "Timeout-Fail-Test", chrono::milliseconds(2000)); // timeout 2000 ms
    run(&MySuite::default_timeout_pass_test, "Default-Timeout-Pass-Test");    // default timeout
    run(&MySuite::default_timeout_fail_test, "Default-Timeout-Fail-Test");    // default timeout

    auto& utest = run(&MySuite::async_test_uncontrolled, "Async-Test-Uncontrolled");
    auto& atest = run(&MySuite::async_test_controlled, "Async-Test").after(utest);
    auto& stest = run(&MySuite::sync_test, "Sync-Test", tthread::chrono::milliseconds(500));
    clone(stest, "Sync-Test-2").after(atest);
    clone(atest, "Async-Test-2");
}

void MySuite::sync_test(const std::string& test_case_name)
{
    FUNCLOG;

    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(500));

    complete_test_case(test_case_name, test_outcome::pass);
}

void MySuite::async_test_controlled(const std::string& test_case_name)
{
    FUNCLOG;

    go_async(
        test_case_name                   // Test case name
        , &MySuite::async_test_func_obj  // Pointer to asynchronous functional object
        // , chrono::milliseconds(1000)     // Timeout for asynchronous functional object
    );
}
void MySuite::async_test_func_obj(const std::string& test_case_name)
{
    FUNCLOG;

    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(2000));

    complete_test_case(test_case_name, test_outcome::pass);
}

void MySuite::async_test_uncontrolled(const std::string& test_case_name)
{
    FUNCLOG;

    thread_data* data = new thread_data;
    data->suite = this;
    data->test_case_name = test_case_name;

    std::shared_ptr<tthread::thread> thread = std::make_shared<tthread::thread>(uncontrolled_test_case_thread, data);
    d_threads.push_back(thread);
}
void uncontrolled_test_case_thread(void* p)
{
    FUNCLOG;

    thread_data* data = (thread_data*)p;

    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(1000));

    data->suite->complete_test_case(data->test_case_name, test_outcome::pass);

    delete data;
}

