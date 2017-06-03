#include "catch.hpp"
#include "suite.hpp"
#include "g2fasth_enums.hpp"
#include "test_run_spec.hpp"
#include "test_run_instance.hpp"

using namespace g2::fasth;

void test_case_pass_thread(void * aArg)
{
    test_run_instance * test_instance = static_cast<test_run_instance*>(aArg);
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(1000));
    test_instance->set_outcome(test_outcome::pass);
}

void test_case_fail_thread(void * aArg)
{
    test_run_instance * test_instance = static_cast<test_run_instance*>(aArg);
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(1000));
    test_instance->set_outcome(test_outcome::fail);
}

class TestAsyncScenarios : public g2::fasth::suite<TestAsyncScenarios> {
public:
    TestAsyncScenarios()
        : suite("TestAsync", g2::fasth::test_order::implied, g2::fasth::log_level::NONE) {
    };
    void setup_test_track_scenario1() override
    {
        run(&TestAsyncScenarios::first_test, "first_test");
    };
    void setup_test_track_scenario2() override
    {
        run(&TestAsyncScenarios::first_test, "first_test");
        run(&TestAsyncScenarios::second_test, "second_test").after_success_of(&TestAsyncScenarios::first_test);
    };
    void setup_test_track_scenario3() override
    {
        run(&TestAsyncScenarios::first_test, "first_test");
        run(&TestAsyncScenarios::third_test, "third_test").after_success_of(&TestAsyncScenarios::first_test);
    };
    void setup_test_track_scenario4() override
    {
        run(&TestAsyncScenarios::third_test, "third_test");
        run(&TestAsyncScenarios::first_test, "first_test").after_success_of(&TestAsyncScenarios::third_test);
    };
    void setup_test_track_scenario5() override
    {
        run(&TestAsyncScenarios::second_test, "second_test");
        run(&TestAsyncScenarios::first_test, "first_test").after_success_of(&TestAsyncScenarios::second_test);
    };
    g2::fasth::test_outcome first_test(test_run_instance &test_instance)
    {
        output += "<A>";
        std::shared_ptr<tthread::thread> ptr = std::make_shared<tthread::thread>(test_case_pass_thread, &test_instance);
        threads.push_back(ptr);
        return test_outcome::by_instance;
    }
    g2::fasth::test_outcome second_test(test_run_instance &test_instance)
    {
        output += "<B>";
        std::shared_ptr<tthread::thread> ptr = std::make_shared<tthread::thread>(test_case_fail_thread, &test_instance);
        threads.push_back(ptr);
        return test_outcome::by_instance;
    }
    g2::fasth::test_outcome third_test(test_run_instance &)
    {
        output += "<C>";
        return test_outcome::pass;
    }
    std::string output;
    std::vector<std::shared_ptr<tthread::thread>> threads;
};

TEST_CASE("Test Suite should run async test in finite time") {
    TestAsyncScenarios test_async;
    test_async.setup_test_track_scenario1();
    test_async.execute();
    auto results = test_async.get_results();
    REQUIRE(results[0].test_outcome() == test_outcome::pass);
}

TEST_CASE("Test Suite should run dependent async test in finite time") {
    TestAsyncScenarios test_async;
    test_async.setup_test_track_scenario2();
    test_async.execute();
    auto results = test_async.get_results();
    REQUIRE(results[0].test_outcome() == test_outcome::pass);
    REQUIRE(results[1].test_outcome() == test_outcome::fail);
}

TEST_CASE("Sync test should run only when dependent async test has finished executing.") {
    TestAsyncScenarios test_async;
    test_async.setup_test_track_scenario3();
    test_async.execute();
    auto results = test_async.get_results();
    REQUIRE(results[0].test_outcome() == test_outcome::pass);
    REQUIRE(results[1].test_outcome() == test_outcome::pass);
    REQUIRE(test_async.output == "<A><C>");
}

TEST_CASE("Async test should run only when dependent sync test has executed.") {
    TestAsyncScenarios test_async;
    test_async.setup_test_track_scenario4();
    test_async.execute();
    auto results = test_async.get_results();
    REQUIRE(results[0].test_outcome() == test_outcome::pass);
    REQUIRE(results[1].test_outcome() == test_outcome::pass);
    REQUIRE(test_async.output == "<C><A>");
}

TEST_CASE("Test should fail and not execute if dependent method (after_sccess_of) fails.") {
    TestAsyncScenarios test_async;
    test_async.setup_test_track_scenario5();
    test_async.execute();
    auto results = test_async.get_results();
    REQUIRE(results[0].test_outcome() == test_outcome::fail);
    REQUIRE(results[1].test_outcome() == test_outcome::fail);
    REQUIRE(test_async.output == "<B>");
}
