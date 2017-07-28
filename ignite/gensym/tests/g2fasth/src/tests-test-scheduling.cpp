#include "catch.hpp"
#include "suite.hpp"

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
    void first_test(const std::string& test_case_name)
    {
        complete_test_case(test_case_name, test_outcome::pass);
    }
    void second_test(const std::string& test_case_name)
    {
        complete_test_case(test_case_name, test_outcome::pass);
    }
    void third_test(const std::string& test_case_name)
    {
        complete_test_case(test_case_name, test_outcome::pass);
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
    void first_test(const std::string& test_case_name)
    {
        complete_test_case(test_case_name, test_outcome::pass);
    }
    void second_test(const std::string& test_case_name)
    {
        complete_test_case(test_case_name, test_outcome::pass);
    }
    void third_test(const std::string& test_case_name)
    {
        complete_test_case(test_case_name, test_outcome::pass);
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
    void first_test(const std::string& test_case_name)
    {
        complete_test_case(test_case_name, test_outcome::pass);
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
    void first_test(const std::string& test_case_name)
    {
        complete_test_case(test_case_name, test_outcome::pass);
    }
};

TEST_CASE("Test suite should throw exception if it detects cycle in after construct") {
    FUNCLOG;
    TestSuiteCycle testsuite;
    REQUIRE_THROWS(testsuite.setup_test_track());
}

TEST_CASE("Test suite should throw exception if it detects cycle in after_success_of construct") {
    FUNCLOG;
    TestSuiteAfterSuccessOfCycle testsuite;
    REQUIRE_THROWS(testsuite.setup_test_track());
}

TEST_CASE("Test suite should throw exception if same test is scheduled as dependent test case in after construct") {
    FUNCLOG;
    TestSuiteSameTestCase testsuite;
    REQUIRE_THROWS(testsuite.setup_test_track());
}

TEST_CASE("Test suite should throw exception if same test is scheduled as dependent test case in after_success_of construct") {
    FUNCLOG;
    TestSuiteSameSuccessTestCase testsuite;
    REQUIRE_THROWS(testsuite.setup_test_track());
}

class ParallelTestCases : public suite<ParallelTestCases> {
public:
    tthread::mutex d_mutex;
    std::string log;
    ParallelTestCases()
        :suite("ParallelTestCases", test_order::implied, log_level::NONE) {
    };
    void test(const std::string& test_case_name)
    {
        add_log(">");
        tthread::this_thread::sleep_for(chrono::milliseconds(3000));
        complete_test_case(test_case_name, test_outcome::pass);
        add_log("<");
    }
    void add_log(const std::string& s)
    {
        tthread::lock_guard<tthread::mutex> lg(d_mutex);
        log += s;
    }
};

TEST_CASE("Parallel test case execution") {
    FUNCLOG;
    ParallelTestCases ts;

    for (long long i=1; i<=4; i++)
        ts.run(&ParallelTestCases::test, std::to_string(i));

    std::shared_ptr<tthread::thread_pool> thread_pool = std::make_shared<tthread::thread_pool>();
    thread_pool->set_thread_limit(4);

    ts.execute(thread_pool);

    REQUIRE(ts.log.length() == 8);
    REQUIRE(ts.log.substr(0,4) == ">>>>");
}
