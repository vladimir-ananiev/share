#include "catch.hpp"
#include "suite.hpp"

using namespace g2::fasth;

class TestJUnit : public g2::fasth::suite<TestJUnit> {
public:
    TestJUnit()
        : suite("TestJUnit", g2::fasth::test_order::implied, g2::fasth::log_level::NONE)
        , first_test_should_fail(false){
    };
    void setup_test_track() override
    {
        run(&TestJUnit::first_test, "first_test");
    };
    void first_test(const std::string& test_case_name)
    {
        if (first_test_should_fail)
        {
            complete_test_case(test_case_name, test_outcome::fail);
        }
        else
        {
            complete_test_case(test_case_name, test_outcome::pass);
        }
    }
    bool first_test_should_fail;
};

TEST_CASE("Test Suite should return JUnit report after all success") {
    FUNCLOG;
    TestJUnit test_junit;
    auto report = test_junit.execute();
    auto results = test_junit.get_results();
    REQUIRE(report == "<testsuite tests=\"1\">\n    <testcase classname=\"TestJUnit.first_test\" name=\"first_test\"/>\n</testsuite>\n");
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].outcome() == g2::fasth::test_outcome::pass);
}

TEST_CASE("Test Suite should return JUnit report after some success") {
    FUNCLOG;
    TestJUnit test_junit;
    test_junit.first_test_should_fail = true;
    auto report = test_junit.execute();
    auto results = test_junit.get_results();
    REQUIRE(report == "<testsuite tests=\"1\">\n    <testcase classname=\"TestJUnit.first_test\" name=\"first_test\">\n        <failure type=\"Fail\">This has failed due to unknow reasons.</failure>\n    </testcase>\n</testsuite>\n");
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].outcome() == g2::fasth::test_outcome::fail);
}