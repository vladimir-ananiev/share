#include "catch.hpp"
#include "suite.hpp"
#include "g2fasth_enums.hpp"
#include "test_run_spec.hpp"

using namespace g2::fasth;

class TestBeforeAfterConstruct : public g2::fasth::suite<TestBeforeAfterConstruct> {
public:
    TestBeforeAfterConstruct()
        : suite("TestBeforeAfterConstruct", g2::fasth::test_order::implied, g2::fasth::log_level::SILENT)
    {
    }
    void before() override
    {
        output.push_back("before");
    };
    void after() override
    {
        output.push_back("after");
    };
    void setup_test_track() override
    {
        run(&TestBeforeAfterConstruct::first_test, "first_test");
        run(&TestBeforeAfterConstruct::second_test, "second_test");
    };
    g2::fasth::test_outcome first_test(test_run_instance &)
    {
        output.push_back("first_test");
        return g2::fasth::test_outcome::pass;
    }
    g2::fasth::test_outcome second_test(test_run_instance &)
    {
        output.push_back("second_test");
        return g2::fasth::test_outcome::pass;
    }
    std::vector<std::string> output;
};

TEST_CASE("Before and after should be called in correct sequence while executing of test case") {
    TestBeforeAfterConstruct testsuite;
    testsuite.execute();
    std::stringstream imploded;
    for (auto i = 0; i < testsuite.output.size(); ++i)
    {
        if (i != 0)
            imploded << ",";
        imploded << testsuite.output[i];
    }
    REQUIRE(imploded.str() == "before,first_test,after,before,second_test,after");
}