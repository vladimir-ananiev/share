#include "catch.hpp"
#include "suite.hpp"
#include "g2fasth_enums.hpp"
#include "test_run_spec.hpp"
#include "test_run_instance.hpp"
#include "tinythread.h"

using namespace g2::fasth;

class TestTimeouts : public g2::fasth::suite<TestTimeouts> {
public:
    TestTimeouts()
        : suite("TestTimeouts", g2::fasth::test_order::implied, g2::fasth::log_level::NONE) {
    };
    TestTimeouts(chrono::milliseconds default_timeout)
        : suite("TestTimeouts", g2::fasth::test_order::implied, g2::fasth::log_level::NONE, "", default_timeout) {
    };

    g2::fasth::test_outcome test_1(test_run_instance &test_instance)
    {
        tthread::this_thread::sleep_for(tthread::chrono::milliseconds(5000));
        return test_outcome::pass;
    }
    g2::fasth::test_outcome test_2(test_run_instance &test_instance)
    {
        tthread::this_thread::sleep_for(tthread::chrono::milliseconds(500));
        return test_outcome::pass;
    }
    g2::fasth::test_outcome test_3(test_run_instance &)
    {
        tthread::this_thread::sleep_for(tthread::chrono::milliseconds(500));
        return test_outcome::pass;
    }
};

TEST_CASE("Timeout is not specified") {
    TestTimeouts test_suite;
    test_suite.run(&TestTimeouts::test_1, "test_1");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].test_outcome() == test_outcome::pass);
}

TEST_CASE("Test is in time") {
    TestTimeouts test_suite;
    test_suite.run(&TestTimeouts::test_2, "test_2", chrono::milliseconds(600));
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].test_outcome() == test_outcome::pass);
}

TEST_CASE("Test is timed out") {
    TestTimeouts test_suite;
    test_suite.run(&TestTimeouts::test_3, "test_3", chrono::milliseconds(400));
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].test_outcome() == test_outcome::fail);
}

TEST_CASE("Default timeout pass test") {
    TestTimeouts test_suite(chrono::milliseconds(600));
    test_suite.run(&TestTimeouts::test_2, "test_2");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].test_outcome() == test_outcome::pass);
}

TEST_CASE("Default timeout fail test") {
    TestTimeouts test_suite(chrono::milliseconds(400));
    test_suite.run(&TestTimeouts::test_2, "test_2");
    test_suite.execute();
    auto results = test_suite.get_results();
    REQUIRE(results[0].test_outcome() == test_outcome::fail);
}

