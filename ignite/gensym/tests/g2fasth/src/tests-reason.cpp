#include "catch.hpp"
#include "suite.hpp"
#include "tinythread.h"

using namespace g2::fasth;

class TestReasons : public g2::fasth::suite<TestReasons> {
public:
    test_run_reason reason1;
    test_run_reason reason2;
    TestReasons()
        : suite("TestReasons", g2::fasth::test_order::implied, g2::fasth::log_level::NONE)
    {
        reason1 = (test_run_reason)-1;
        reason2 = (test_run_reason)-1;
    };

    void reason_start(const std::string& test_case_name, test_run_reason reason)
    {
        reason1 = reason;
        complete_test_case(test_case_name, test_outcome::pass);
    }
    void reason_async_with_separate_func_obj(const std::string& test_case_name, test_run_reason reason)
    {
        reason1 = reason;
        go_async(test_case_name
            , &TestReasons::async_func_obj
        );
    }
    void async_func_obj(const std::string& test_case_name, test_run_reason reason)
    {
        reason2 = reason;
        complete_test_case(test_case_name, test_outcome::pass);
    }
    void reason_timeout_with_separate_func_obj(const std::string& test_case_name, test_run_reason reason)
    {
        reason1 = reason;
        start_timer(test_case_name
            , chrono::milliseconds(100)
            , &TestReasons::timer_func_obj
        );
    }
    void timer_func_obj(const std::string& test_case_name, test_run_reason reason)
    {
        reason2 = reason;
        complete_test_case(test_case_name, test_outcome::pass);
    }
    void reason_async_with_same_func_obj(const std::string& test_case_name, test_run_reason reason)
    {
        static bool start = true;
        if (start)
        {
            reason1 = reason;
            start = false;
            return go_async(test_case_name);
        }
        reason2 = reason;
        complete_test_case(test_case_name, test_outcome::pass);
    }
    void reason_timeout_with_same_func_obj(const std::string& test_case_name, test_run_reason reason)
    {
        static bool start = true;
        if (start)
        {
            reason1 = reason;
            start = false;
            return start_timer(test_case_name, chrono::milliseconds(100));
        }
        reason2 = reason;
        complete_test_case(test_case_name, test_outcome::pass);
    }
};

TEST_CASE("Test reasons - start") {
    TestReasons test_suite;
    test_suite.run(&TestReasons::reason_start, "reason_start");
    test_suite.execute();
    REQUIRE(test_suite.reason1 == test_run_reason::start);
}

TEST_CASE("Test reasons - async with separate func obj") {
    TestReasons test_suite;
    test_suite.run(&TestReasons::reason_async_with_separate_func_obj, "reason_async_with_separate_func_obj");
    test_suite.execute();
    REQUIRE(test_suite.reason1 == test_run_reason::start);
    REQUIRE(test_suite.reason2 == test_run_reason::async);
}

TEST_CASE("Test reasons - timeout with separate func obj") {
    TestReasons test_suite;
    test_suite.run(&TestReasons::reason_timeout_with_separate_func_obj, "reason_timeout_with_separate_func_obj");
    test_suite.execute();
    REQUIRE(test_suite.reason1 == test_run_reason::start);
    REQUIRE(test_suite.reason2 == test_run_reason::timeout);
}

TEST_CASE("Test reasons - async with the same func obj") {
    TestReasons test_suite;
    test_suite.run(&TestReasons::reason_async_with_same_func_obj, "reason_async_with_same_func_obj");
    test_suite.execute();
    REQUIRE(test_suite.reason1 == test_run_reason::start);
    REQUIRE(test_suite.reason2 == test_run_reason::async);
}

TEST_CASE("Test reasons - timeout with the same func obj") {
    TestReasons test_suite;
    test_suite.run(&TestReasons::reason_timeout_with_same_func_obj, "reason_timeout_with_same_func_obj");
    test_suite.execute();
    REQUIRE(test_suite.reason1 == test_run_reason::start);
    REQUIRE(test_suite.reason2 == test_run_reason::timeout);
}

