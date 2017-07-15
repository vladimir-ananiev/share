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

class TestTwo : public suite<TestTwo> {
public:
    TestTwo()
        : suite("TestTwo", test_order::implied, log_level::SILENT) {
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

class SimpleTestSuite : public suite<SimpleTestSuite> {
    std::string str;
    int sleep;
    bool log;
public:
    SimpleTestSuite(const std::string& str, int sleep, bool log=false)
        : suite("SimpleTestSuite"+str, test_order::implied, log_level::SILENT)
        , str(str), sleep(sleep), log(log)
    {
    }
    void setup_test_track() override
    {
        //if (log) printf("%d) Start %s\n", (int)GetTickCount(), str.c_str());
        tthread::this_thread::sleep_for(chrono::milliseconds(sleep));
        //if (log) printf("%d) After %d sleep - %s\n", (int)GetTickCount(), sleep, str.c_str());
        output += str;
    };
};

TEST_CASE("After construct should schedule test suites correctly") {
    output.clear();
    auto test_one = std::make_shared<TestOne>();
    auto test_two = std::make_shared<TestTwo>();
    test_agent agent;
    agent.schedule_suite(test_two);
    agent.schedule_suite(test_one, test_two);
    agent.execute();
    REQUIRE(output == "TwoOne");
}

TEST_CASE("After construct should throw exception if same suite is scheduled again.") {
    auto test_one = std::make_shared<TestOne>();
    auto test_two = std::make_shared<TestTwo>();
    test_agent agent;
    agent.schedule_suite(test_two);
    agent.schedule_suite(test_one, test_two);
    REQUIRE_THROWS(agent.schedule_suite(test_two));
}

TEST_CASE("After construct should throw exception if same suite is scheduled as dependent suite.") {
    auto test_one = std::make_shared<TestOne>();
    test_agent agent;
    REQUIRE_THROWS(agent.schedule_suite(test_one, test_one));
}

TEST_CASE("Run all not dependent suites one by one") {
    output.clear();
    std::string expected;
    test_agent agent(test_order::implied);
    for (int i=0; i<10; i++)
    {
        std::string str = std::to_string((long long)i);
        expected += str;
        agent.schedule_suite(std::make_shared<SimpleTestSuite>(str,0));
    }
    agent.execute();
    REQUIRE(output == expected);
}

TEST_CASE("Run all not dependent suites randomly") {
    output.clear();
    std::string expected;
    test_agent agent(test_order::random);
    for (int i=0; i<10; i++)
    {
        std::string str = std::to_string((long long)i);
        expected += str;
        agent.schedule_suite(std::make_shared<SimpleTestSuite>(str,0));
    }
    agent.execute();
    REQUIRE(output.length() == expected.length());
    REQUIRE(output != expected);
}

TEST_CASE("Run all not dependent suites in parallel") {
    output.clear();
    std::string expected;
    test_agent agent;
    for (int i=0; i<10; i++)
    {
        expected += std::to_string((long long)(9-i));
        agent.schedule_suite(std::make_shared<SimpleTestSuite>(std::to_string((long long)i),(10-i)*100));
    }
    agent.set_concurrency(10);
    agent.execute(true);
    REQUIRE(output == expected);
}

TEST_CASE("Run suites in parallel, when some suites depend on the other ones") {

    std::shared_ptr<base_suite> ts0 = std::make_shared<SimpleTestSuite>("0",100,true);
    std::shared_ptr<base_suite> ts1 = std::make_shared<SimpleTestSuite>("1",200,true);
    std::shared_ptr<base_suite> ts2 = std::make_shared<SimpleTestSuite>("2",300,true);
    std::shared_ptr<base_suite> ts3 = std::make_shared<SimpleTestSuite>("3",400,true);
    std::shared_ptr<base_suite> ts4 = std::make_shared<SimpleTestSuite>("4",500,true);
    std::shared_ptr<base_suite> ts5 = std::make_shared<SimpleTestSuite>("5",200,true);
    std::shared_ptr<base_suite> ts6 = std::make_shared<SimpleTestSuite>("6",250,true);
    std::shared_ptr<base_suite> ts7 = std::make_shared<SimpleTestSuite>("7",350,true);
    std::shared_ptr<base_suite> ts8 = std::make_shared<SimpleTestSuite>("8",450,true);
    std::shared_ptr<base_suite> ts9 = std::make_shared<SimpleTestSuite>("9",550,true);

    test_agent agent;
    agent.set_concurrency(10);

    agent.schedule_suite(ts0);
    agent.schedule_suite(ts1,ts0);
    agent.schedule_suite(ts2,ts0);
    agent.schedule_suite(ts3,ts0);
    agent.schedule_suite(ts4,ts0);
    agent.schedule_suite(ts5);
    agent.schedule_suite(ts6,ts5);
    agent.schedule_suite(ts7,ts5);
    agent.schedule_suite(ts8,ts5);
    agent.schedule_suite(ts9,ts5);

    output.clear();
    agent.execute(true);

    std::string expected = "0512637489";
    REQUIRE(output == expected);
}

