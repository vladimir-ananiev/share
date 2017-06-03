#include "catch.hpp"
#include "suite.hpp"
#include "g2fasth_enums.hpp"
#include "test_run_spec.hpp"
#include "test_run_instance.hpp"

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
    g2::fasth::test_outcome first_test(test_run_instance &)
    {
        if (first_test_should_fail)
        {
            return g2::fasth::test_outcome::fail;
        }
        else
        {
            return g2::fasth::test_outcome::pass;
        }
    }
    bool first_test_should_fail;
};

TEST_CASE("Test Suite should return JUnit report after all success") {
    TestJUnit test_junit;
    auto report = test_junit.execute();
    auto results = test_junit.get_results();
    REQUIRE(report == "<testsuite tests=\"1\">\n    <testcase classname=\"TestJUnit.first_test\" name=\"first_test\"/>\n</testsuite>\n");
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].test_outcome() == g2::fasth::test_outcome::pass);
}

TEST_CASE("Test Suite should return JUnit report after some success") {
    TestJUnit test_junit;
    test_junit.first_test_should_fail = true;
    auto report = test_junit.execute();
    auto results = test_junit.get_results();
    REQUIRE(report == "<testsuite tests=\"1\">\n    <testcase classname=\"TestJUnit.first_test\" name=\"first_test\">\n        <failure type=\"Fail\">This has failed due to unknow reasons.</failure>\n    </testcase>\n</testsuite>\n");
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].test_outcome() == g2::fasth::test_outcome::fail);
}