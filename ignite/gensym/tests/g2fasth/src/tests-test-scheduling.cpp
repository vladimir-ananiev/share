#include "catch.hpp"
#include "suite.hpp"
#include "g2fasth_enums.hpp"
#include "test_run_spec.hpp"

using namespace g2::fasth;

class TestSuiteCycle : public suite<TestSuiteCycle> {
public:
    TestSuiteCycle()
        :suite("TestSuiteCycle", test_order::implied, log_level::NONE) {
    };
    void setup_test_track() override
    {
        run(&TestSuiteCycle::first_test, "first_test");
        run(&TestSuiteCycle::second_test, "second_test").after(&TestSuiteCycle::first_test);
        run(&TestSuiteCycle::first_test, "third_test").after(&TestSuiteCycle::second_test);
        run(&TestSuiteCycle::first_test, "first_test").after(&TestSuiteCycle::third_test);
    };
    test_outcome first_test(test_run_instance &)
    {
        return test_outcome::pass;
    }
    test_outcome second_test(test_run_instance &)
    {
        return test_outcome::pass;
    }
    test_outcome third_test(test_run_instance &)
    {
        return test_outcome::pass;
    }
};

class TestSuiteAfterSuccessOfCycle : public suite<TestSuiteAfterSuccessOfCycle> {
public:
    TestSuiteAfterSuccessOfCycle()
        :suite("TestSuiteAfterSuccessOfCycle", test_order::implied, log_level::SILENT) {
    };
    void setup_test_track() override
    {
        run(&TestSuiteAfterSuccessOfCycle::first_test, "first_test");
        run(&TestSuiteAfterSuccessOfCycle::second_test, "second_test").after_success_of(&TestSuiteAfterSuccessOfCycle::first_test);
        run(&TestSuiteAfterSuccessOfCycle::first_test, "third_test").after_success_of(&TestSuiteAfterSuccessOfCycle::second_test);
        run(&TestSuiteAfterSuccessOfCycle::first_test, "first_test").after_success_of(&TestSuiteAfterSuccessOfCycle::third_test);
    };
    test_outcome first_test(test_run_instance &)
    {
        return test_outcome::pass;
    }
    test_outcome second_test(test_run_instance &)
    {
        return test_outcome::pass;
    }
    test_outcome third_test(test_run_instance &)
    {
        return test_outcome::pass;
    }
};

class TestSuiteSameTestCase : public suite<TestSuiteSameTestCase> {
public:
    TestSuiteSameTestCase()
        :suite("TestSuiteSameTestCase", test_order::implied, log_level::SILENT) {
    };
    void setup_test_track() override
    {
        run(&TestSuiteSameTestCase::first_test, "first_test");
        run(&TestSuiteSameTestCase::first_test, "first_test").after(&TestSuiteSameTestCase::first_test);
    };
    test_outcome first_test(test_run_instance &)
    {
        return test_outcome::pass;
    }
};

class TestSuiteSameSuccessTestCase : public suite<TestSuiteSameSuccessTestCase> {
public:
    TestSuiteSameSuccessTestCase()
        :suite("TestSuiteSameSuccessTestCase", test_order::implied, log_level::NONE) {
    };
    void setup_test_track() override
    {
        run(&TestSuiteSameSuccessTestCase::first_test, "first_test");
        run(&TestSuiteSameSuccessTestCase::first_test, "first_test").after_success_of(&TestSuiteSameSuccessTestCase::first_test);
    };
    test_outcome first_test(test_run_instance &)
    {
        return test_outcome::pass;
    }
};

TEST_CASE("Test suite should throw exception if it detects cycle in after construct") {
    TestSuiteCycle testsuite;
    REQUIRE_THROWS(testsuite.setup_test_track());
}

TEST_CASE("Test suite should throw exception if it detects cycle in after_success_of construct") {
    TestSuiteAfterSuccessOfCycle testsuite;
    REQUIRE_THROWS(testsuite.setup_test_track());
}

TEST_CASE("Test suite should throw exception if same test is scheduled as dependent test case in after construct") {
    TestSuiteSameTestCase testsuite;
    REQUIRE_THROWS(testsuite.setup_test_track());
}

TEST_CASE("Test suite should throw exception if same test is scheduled as dependent test case in after_success_of construct") {
    TestSuiteSameSuccessTestCase testsuite;
    REQUIRE_THROWS(testsuite.setup_test_track());
}