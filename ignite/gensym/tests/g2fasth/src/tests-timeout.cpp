#include "catch.hpp"
#include "suite.hpp"
#include "tinythread.h"

using namespace g2::fasth;

class TestTimeouts : public g2::fasth::suite<TestTimeouts> {
public:
    bool use_default_timeout;
    TestTimeouts()
        : suite("TestTimeouts", g2::fasth::test_order::implied, g2::fasth::log_level::NONE), use_default_timeout(true)
    {
    };
    TestTimeouts(chrono::milliseconds default_timeout)
        : suite("TestTimeouts", g2::fasth::test_order::implied, g2::fasth::log_level::NONE, "", default_timeout), use_default_timeout(true)
    {
    };

    void test_1(const std::string& test_case_name)
    {
        tthread::this_thread::sleep_for(tthread::chrono::milliseconds(2000));
        
        complete_test_case(test_case_name, test_outcome::pass);
    }
    void test_2(const std::string& test_case_name)
    {
        tthread::this_thread::sleep_for(tthread::chrono::milliseconds(500));
        
        complete_test_case(test_case_name, test_outcome::pass);
    }
    void test_3(const std::string& test_case_name)
    {
        tthread::this_thread::sleep_for(tthread::chrono::milliseconds(500));
        
        complete_test_case(test_case_name, test_outcome::pass);
    }
    void async_test_1(const std::string& test_case_name)
    {
        if (use_default_timeout)
            go_async(test_case_name, &TestTimeouts::async_test_1_func_obj);
        else
            go_async(test_case_name, &TestTimeouts::async_test_1_func_obj, chrono::milliseconds(1000));
    }
    void async_test_1_func_obj(const std::string& test_case_name)
    {
        tthread::this_thread::sleep_for(tthread::chrono::milliseconds(800));

        complete_test_case(test_case_name, test_outcome::pass);
    }
    void async_test_2(const std::string& test_case_name)
    {
        if (use_default_timeout)
            go_async(test_case_name, &TestTimeouts::async_test_2_func_obj);
        else
            go_async(test_case_name, &TestTimeouts::async_test_2_func_obj, chrono::milliseconds(400));
    }
    void async_test_2_func_obj(const std::string& test_case_name)
    {
        tthread::this_thread::sleep_for(tthread::chrono::milliseconds(600));

        complete_test_case(test_case_name, test_outcome::pass);
    }
};

TEST_CASE("Timeout is not specified") {
    TestTimeouts test_suite;
    test_suite.run(&TestTimeouts::test_1, "test_1");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::pass);
}

TEST_CASE("Test is in time") {
    TestTimeouts test_suite;
    test_suite.run(&TestTimeouts::test_2, "test_2", chrono::milliseconds(600));
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::pass);
}

TEST_CASE("Test is timed out") {
    TestTimeouts test_suite;
    test_suite.run(&TestTimeouts::test_3, "test_3", chrono::milliseconds(400));
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::fail);
}

TEST_CASE("Default timeout pass test") {
    TestTimeouts test_suite(chrono::milliseconds(600));
    test_suite.run(&TestTimeouts::test_2, "test_2");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::pass);
}

TEST_CASE("Default timeout fail test") {
    TestTimeouts test_suite(chrono::milliseconds(400));
    test_suite.run(&TestTimeouts::test_2, "test_2");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::fail);
}

TEST_CASE("Async test, default timeout pass test") {
    TestTimeouts test_suite(chrono::milliseconds(1000));
    test_suite.run(&TestTimeouts::async_test_1, "async_test_1");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::pass);
}

TEST_CASE("Async test, default timeout fail test") {
    TestTimeouts test_suite(chrono::milliseconds(500));
    test_suite.run(&TestTimeouts::async_test_2, "async_test_2");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::fail);
}

TEST_CASE("Async test, timeout pass test") {
    TestTimeouts test_suite;
    test_suite.use_default_timeout = false;
    test_suite.run(&TestTimeouts::async_test_1, "async_test_1");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::pass);
}

TEST_CASE("Async test, timeout fail test") {
    TestTimeouts test_suite;
    test_suite.use_default_timeout = false;
    test_suite.run(&TestTimeouts::async_test_2, "async_test_2");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].outcome() == test_outcome::fail);
}
