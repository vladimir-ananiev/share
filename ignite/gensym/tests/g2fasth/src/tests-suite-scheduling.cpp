#include "catch.hpp"
#include "suite.hpp"
#include "test_agent.hpp"

using namespace g2::fasth;

std::string output = "";

class TestOne : public suite<TestOne> {
public:
    TestOne()
        : suite("TestOne", test_order::implied, log_level::SILENT) {
    }
    void setup_test_track() override
    {
        output += "One";
        run(&TestOne::first_test, "first_test");
    };
    void first_test(const std::string& test_case_name)
    {
        complete_test_case(test_case_name, test_outcome::pass);
    }
};

class TestTwo : public g2::fasth::suite<TestTwo> {
public:
    TestTwo()
        : suite("TestTwo", g2::fasth::test_order::implied, g2::fasth::log_level::SILENT) {
    }
    void setup_test_track() override
    {
        output += "Two";
        run(&TestTwo::first_test, "first_test");
    };
    void first_test(const std::string& test_case_name)
    {
        complete_test_case(test_case_name, test_outcome::pass);
    }
};

TEST_CASE("After construct should schedule test suites correctly") {
    auto test_one = std::make_shared<TestOne>();
    auto test_two = std::make_shared<TestTwo>();
    g2::fasth::test_agent agent;
    agent.run_suite(test_one).after(test_two);
    agent.execute();
    REQUIRE(output == "TwoOne");
}

TEST_CASE("After construct should throw exception if same suite is scheduled again.") {
    auto test_one = std::make_shared<TestOne>();
    auto test_two = std::make_shared<TestTwo>();
    g2::fasth::test_agent agent;
    agent.run_suite(test_one).after(test_two);
    REQUIRE_THROWS(agent.run_suite(test_two));
}

TEST_CASE("After construct should throw exception if suite is scheduled without dependent suite.") {
    auto test_one = std::make_shared<TestOne>();
    g2::fasth::test_agent agent;
    REQUIRE_THROWS(agent.after(test_one));
}

TEST_CASE("After construct should throw exception if same suite is scheduled as dependent suite.") {
    auto test_one = std::make_shared<TestOne>();
    g2::fasth::test_agent agent;
    REQUIRE_THROWS(agent.run_suite(test_one).after(test_one));
}