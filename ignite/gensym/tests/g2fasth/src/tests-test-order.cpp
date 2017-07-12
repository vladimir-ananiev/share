#include "catch.hpp"
#include "suite.hpp"
#include "tinythread.h"

using namespace g2::fasth;

class TestOrders : public g2::fasth::suite<TestOrders> {
public:
    std::string log;
    std::string implied_log;
    TestOrders(test_order order)
        : suite("TestOrders", order, log_level::NONE)
    {
        implied_log = "0123456789";
    };

    void test(const std::string& test_case_name, test_run_reason reason)
    {
        log += test_case_name;
        complete_test_case(test_case_name, test_outcome::pass);
    }
};

TEST_CASE("Test order - implied") {
    TestOrders test_suite(test_order::implied);
    test_suite.run(&TestOrders::test, "0");
    test_suite.run(&TestOrders::test, "1");
    test_suite.run(&TestOrders::test, "2");
    test_suite.run(&TestOrders::test, "3");
    test_suite.run(&TestOrders::test, "4");
    test_suite.run(&TestOrders::test, "5");
    test_suite.run(&TestOrders::test, "6");
    test_suite.run(&TestOrders::test, "7");
    test_suite.run(&TestOrders::test, "8");
    test_suite.run(&TestOrders::test, "9");
    test_suite.execute();
    REQUIRE(test_suite.log == test_suite.implied_log);
}

TEST_CASE("Test order - random") {
    TestOrders test_suite(test_order::random);
    test_suite.run(&TestOrders::test, "0");
    test_suite.run(&TestOrders::test, "1");
    test_suite.run(&TestOrders::test, "2");
    test_suite.run(&TestOrders::test, "3");
    test_suite.run(&TestOrders::test, "4");
    test_suite.run(&TestOrders::test, "5");
    test_suite.run(&TestOrders::test, "6");
    test_suite.run(&TestOrders::test, "7");
    test_suite.run(&TestOrders::test, "8");
    test_suite.run(&TestOrders::test, "9");
    test_suite.execute();
    REQUIRE(test_suite.log.length() == test_suite.implied_log.length());
    REQUIRE(test_suite.log != test_suite.implied_log);
}

