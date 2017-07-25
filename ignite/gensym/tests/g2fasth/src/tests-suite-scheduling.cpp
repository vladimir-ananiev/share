#include "catch.hpp"
#include "suite.hpp"
#include "test_agent.hpp"

using namespace g2::fasth;

std::string output = "";
tthread::mutex mutex;

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
public:
    SimpleTestSuite(const std::string& str, int sleep)
        : suite("SimpleTestSuite"+str, test_order::implied, log_level::SILENT)
        , str(str), sleep(sleep)
    {
    }
    void setup_test_track() override
    {
        {
            tthread::lock_guard<tthread::mutex> lg(mutex);
            output += str + ">";
        }
        //printf("%d) Start %s\n", (int)GetTickCount(), str.c_str());
        tthread::this_thread::sleep_for(chrono::milliseconds(sleep));
        //printf("%d) After %d sleep - %s\n", (int)GetTickCount(), sleep, str.c_str());
        {
            tthread::lock_guard<tthread::mutex> lg(mutex);
            output += str + "<";
        }
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
        expected += str + ">" + str + "<";
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
        expected += str + ">" + str + "<";
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
    int count = 4;
    for (int i=0; i<count; i++)
    {
        std::string str = std::to_string((long long)i);
        agent.schedule_background_suite(std::make_shared<SimpleTestSuite>(str, 3000));
    }
    agent.set_concurrency(count);
    agent.set_sleep_quantum(chrono::milliseconds(10));
    agent.execute();
    REQUIRE(output.length() == count*4);
    REQUIRE(output.substr(1,1) == ">");
    REQUIRE(output.substr(3,1) == ">");
    REQUIRE(output.substr(5,1) == ">");
    REQUIRE(output.substr(7,1) == ">");
}

TEST_CASE("Run all suites in parallel, when some suites depend on the other ones") {

    std::shared_ptr<base_suite> ts0 = std::make_shared<SimpleTestSuite>("0",1000);
    std::shared_ptr<base_suite> ts1 = std::make_shared<SimpleTestSuite>("1",1000);
    std::shared_ptr<base_suite> ts2 = std::make_shared<SimpleTestSuite>("2",1000);
    std::shared_ptr<base_suite> ts3 = std::make_shared<SimpleTestSuite>("3",1000);
    std::shared_ptr<base_suite> ts4 = std::make_shared<SimpleTestSuite>("4",1000);
    std::shared_ptr<base_suite> ts5 = std::make_shared<SimpleTestSuite>("5",1000);
    std::shared_ptr<base_suite> ts6 = std::make_shared<SimpleTestSuite>("6",1000);
    std::shared_ptr<base_suite> ts7 = std::make_shared<SimpleTestSuite>("7",1000);
    std::shared_ptr<base_suite> ts8 = std::make_shared<SimpleTestSuite>("8",1000);
    std::shared_ptr<base_suite> ts9 = std::make_shared<SimpleTestSuite>("9",1000);

    test_agent agent;
    agent.set_concurrency(10);
    agent.set_sleep_quantum(chrono::milliseconds(10));

    agent.schedule_background_suite(ts0);
    agent.schedule_background_suite(ts1,ts0);
    agent.schedule_background_suite(ts2,ts0);
    agent.schedule_background_suite(ts3,ts0);
    agent.schedule_background_suite(ts4,ts0);
    agent.schedule_background_suite(ts5);
    agent.schedule_background_suite(ts6,ts5);
    agent.schedule_background_suite(ts7,ts5);
    agent.schedule_background_suite(ts8,ts5);
    agent.schedule_background_suite(ts9,ts5);

    output.clear();
    agent.execute();

    REQUIRE(output.length() == 40);
    REQUIRE((output.substr(0,4)=="0>5>" || output.substr(0,4)=="5>0>"));
}

TEST_CASE("Run not dependent suites one by one and in parallel") {
    output.clear();
    std::string expected;
    test_agent agent(test_order::implied);
    agent.set_concurrency(5);
    agent.set_sleep_quantum(chrono::milliseconds(10));
    for (int i=0; i<5; i++)
    {
        std::string str = std::to_string((long long)i);
        expected += str;
        agent.schedule_suite(std::make_shared<SimpleTestSuite>(str,0));
    }
    for (int i=5; i<10; i++)
    {
        std::string str = std::to_string((long long)i);
        expected += str;
        agent.schedule_background_suite(std::make_shared<SimpleTestSuite>(str,3000));
    }
    agent.execute();
    std::string substr = output.substr(29,10);
    REQUIRE(output.length() == 40);
    REQUIRE(substr[0] =='<');
    REQUIRE(substr[2] =='<');
    REQUIRE(substr[4] =='<');
    REQUIRE(substr[6] =='<');
    REQUIRE(substr[8] =='<');
}

TEST_CASE("Run dependent suites in parallel and one by one") {
    std::shared_ptr<base_suite> ts0 = std::make_shared<SimpleTestSuite>("0",0);
    std::shared_ptr<base_suite> ts1 = std::make_shared<SimpleTestSuite>("1",0);
    std::shared_ptr<base_suite> ts2 = std::make_shared<SimpleTestSuite>("2",0);
    std::shared_ptr<base_suite> ts3 = std::make_shared<SimpleTestSuite>("3",0);
    std::shared_ptr<base_suite> ts4 = std::make_shared<SimpleTestSuite>("4",0);
    std::shared_ptr<base_suite> ts5 = std::make_shared<SimpleTestSuite>("5",0);
    std::shared_ptr<base_suite> ts6 = std::make_shared<SimpleTestSuite>("6",0);
    std::shared_ptr<base_suite> ts7 = std::make_shared<SimpleTestSuite>("7",0);
    std::shared_ptr<base_suite> ts8 = std::make_shared<SimpleTestSuite>("8",0);
    std::shared_ptr<base_suite> ts9 = std::make_shared<SimpleTestSuite>("9",0);

    test_agent agent;
    agent.set_concurrency(5);
    agent.set_sleep_quantum(chrono::milliseconds(10));

    agent.schedule_background_suite(ts0);
    agent.schedule_background_suite(ts1,ts0);
    agent.schedule_background_suite(ts2,ts1);
    agent.schedule_background_suite(ts3,ts2);
    agent.schedule_background_suite(ts4,ts3);
    agent.schedule_suite(ts5,ts4);
    agent.schedule_suite(ts6,ts5);
    agent.schedule_suite(ts7,ts6);
    agent.schedule_suite(ts8,ts7);
    agent.schedule_suite(ts9,ts8);

    output.clear();
    agent.execute();

    std::string expected = "0>0<1>1<2>2<3>3<4>4<5>5<6>6<7>7<8>8<9>9<";
    REQUIRE(output == expected);
}


