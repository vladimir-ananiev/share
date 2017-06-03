#include "catch.hpp"
#include "suite.hpp"
#include "g2fasth_enums.hpp"

class TestSuiteComplexCycle : public g2::fasth::suite<TestSuiteComplexCycle> {
public:
    TestSuiteComplexCycle()
        :suite("TestSuiteComplexCycle", g2::fasth::test_order::implied, g2::fasth::log_level::SILENT) {
    };
    void setup_test_track() override
    {
        run(&TestSuiteComplexCycle::first_test, "first_test");
        run(&TestSuiteComplexCycle::second_test, "second_test").after(&TestSuiteComplexCycle::first_test);
        run(&TestSuiteComplexCycle::third_test, "third_test").after(&TestSuiteComplexCycle::second_test);
        run(&TestSuiteComplexCycle::first_test, "first_test").after_success_of(&TestSuiteComplexCycle::third_test);
    };
    g2::fasth::test_outcome first_test(g2::fasth::test_run_instance &)
    {
        return g2::fasth::test_outcome::pass;
    }
    g2::fasth::test_outcome second_test(g2::fasth::test_run_instance &)
    {
        return g2::fasth::test_outcome::pass;
    }
    g2::fasth::test_outcome third_test(g2::fasth::test_run_instance &)
    {
        return g2::fasth::test_outcome::pass;
    }
};

TEST_CASE("Test suite should throw exception if it detects complex cycle") {
    TestSuiteComplexCycle testsuite;
    REQUIRE_THROWS(testsuite.setup_test_track());
}