#include "catch.hpp"
#include "suite.hpp"
#include "tinythread.h"

#ifndef WIN32
#include <sys/time.h>
unsigned GetTickCount()
{
    struct timeval tv;
    if(gettimeofday(&tv, NULL) != 0)
        return 0;
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
#endif

using namespace g2::fasth;

class ScopeLog
{
    std::string name, data;
public:
    ScopeLog(const std::string& name, const std::string& data=""): name(name), data(data)
    {
        if (!enabled) return;
        printf("%08u %s (%s) ENTER\n", GetTickCount(), this->name.c_str(), this->data.c_str());
    }
    ~ScopeLog()
    {
        if (!enabled) return;
        printf("%08u %s (%s) LEAVE\n", GetTickCount(), this->name.c_str(), this->data.c_str());
    }
    static bool enabled;
};
bool ScopeLog::enabled = false;
#define FUNCLOG ScopeLog __func_log__(__FUNCTION__)
#define FUNCLOG_DATA(data) ScopeLog __func_log__(__FUNCTION__, data)

class TestTimeouts : public g2::fasth::suite<TestTimeouts> {
public:
    int sleep_time;
    int async_timeout;
    TestTimeouts(const char* name="")
        : suite(name, g2::fasth::test_order::implied, g2::fasth::log_level::NONE)
    {
        async_timeout = 0; // default will be used
        sleep_time = 1000;
    };
    TestTimeouts(chrono::milliseconds default_timeout, const char* name="")
        : suite(name, g2::fasth::test_order::implied, g2::fasth::log_level::NONE, "", default_timeout)
    {
        async_timeout = 0; // default will be used
        sleep_time = 1000;
    };

    void sync_test(const std::string& test_case_name)
    {
        FUNCLOG_DATA(get_suite_name().c_str());

        tthread::this_thread::sleep_for(tthread::chrono::milliseconds(sleep_time));
        
        complete_test_case(test_case_name, test_outcome::pass);
    }
    void async_test(const std::string& test_case_name)
    {
        FUNCLOG_DATA(get_suite_name().c_str());

        go_async(test_case_name, &TestTimeouts::sync_test, chrono::milliseconds(async_timeout));
    }
};

TEST_CASE("Timeout is not specified") {
    TestTimeouts test_suite("Timeout is not specified");
    test_suite.run(&TestTimeouts::sync_test, "sync_test");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::pass);
}

TEST_CASE("Test is in time") {
    TestTimeouts test_suite("Test is in time");
    test_suite.sleep_time = 500;
    test_suite.run(&TestTimeouts::sync_test, "sync_test", chrono::milliseconds(1000));
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::pass);
}

TEST_CASE("Test is timed out") {
    TestTimeouts test_suite("Test is timed out");
    test_suite.sleep_time = 1000;
    test_suite.run(&TestTimeouts::sync_test, "sync_test", chrono::milliseconds(500));
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::fail);
}

TEST_CASE("Default timeout pass test") {
    TestTimeouts test_suite(chrono::milliseconds(1000), "Default timeout pass test");
    test_suite.sleep_time = 500;
    test_suite.run(&TestTimeouts::sync_test, "sync_test");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::pass);
}

TEST_CASE("Default timeout fail test") {
    TestTimeouts test_suite(chrono::milliseconds(500), "Default timeout fail test");
    test_suite.sleep_time = 1000;
    test_suite.run(&TestTimeouts::sync_test, "sync_test");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::fail);
}

TEST_CASE("Async test, default timeout pass test") {
    ScopeLog::enabled = true;
    {
        FUNCLOG;
        TestTimeouts test_suite(chrono::milliseconds(1000), "Async test, default timeout pass test");
        test_suite.sleep_time = 500;
        test_suite.run(&TestTimeouts::async_test, "async_test");
        test_suite.execute();
        auto results = test_suite.get_results();
        REQUIRE(results[0].outcome() == test_outcome::pass);
    }
    ScopeLog::enabled = false;
}

TEST_CASE("Async test, default timeout fail test") {
    TestTimeouts test_suite(chrono::milliseconds(500), "Async test, default timeout fail test");
    test_suite.sleep_time = 1000;
    test_suite.run(&TestTimeouts::async_test, "async_test");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::fail);
}

TEST_CASE("Async test, timeout pass test") {
    TestTimeouts test_suite("Async test, timeout pass test");
    test_suite.sleep_time = 500;
    test_suite.async_timeout = 1000;
    test_suite.run(&TestTimeouts::async_test, "async_test");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::pass);
}

TEST_CASE("Async test, timeout fail test") {
    TestTimeouts test_suite("Async test, timeout fail test");
    test_suite.sleep_time = 1000;
    test_suite.async_timeout = 500;
    test_suite.run(&TestTimeouts::async_test, "async_test");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::fail);
}
