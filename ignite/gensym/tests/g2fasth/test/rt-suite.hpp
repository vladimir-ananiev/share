#pragma once
#include "suite.hpp"
#include "libgsi.hpp"

using namespace std;
using namespace g2::fasth;

class RegTestSuite : public suite<RegTestSuite>
{
public:
    RegTestSuite(string suite_name): suite(suite_name, test_order::implied, g2::fasth::log_level::VERBOSE)
        , gsi(libgsi::getInstance())
    {
    }
    ~RegTestSuite()
    {
        //while (d_threads.size())
        //{
        //    d_threads.front()->join();
        //    d_threads.pop_front();
        //}
    }
    void test_3227(const string&);
    void test_3228(const string&);
    void test_3229(const string&);
    void test_3230(const string&);

private:
    libgsi& gsi;
    //void before() override;
    //void after() override;
    void setup_test_track() override;
    //list<shared_ptr<tthread::thread>> d_threads;
};
