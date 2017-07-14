#include <cstdio>
#include "rt-suite.hpp"

//#ifndef WIN32
//#include <sys/time.h>
//unsigned GetTickCount()
//{
//        struct timeval tv;
//        if(gettimeofday(&tv, NULL) != 0)
//                return 0;
//
//        return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
//}
//#endif
//
//class ScopeLog
//{
//    std::string name;
//public:
//    ScopeLog(const std::string& name): name(name)
//    {
//        printf("%08u %s ENTER\n", GetTickCount(), this->name.c_str());
//    }
//    ~ScopeLog()
//    {
//        printf("%08u %s LEAVE\n", GetTickCount(), this->name.c_str());
//    }
//};
//#define FUNCLOG ScopeLog __func_log__(__FUNCTION__)

void RegTestSuite::setup_test_track()
{
    if (get_suite_name() == "3227")
        run(&RegTestSuite::test_3227, "Test-3227");
    else if (get_suite_name() == "3228")
        run(&RegTestSuite::test_3228, "Test-3228");
    else if (get_suite_name() == "3229")
        run(&RegTestSuite::test_3229, "Test-3229");
    else if (get_suite_name() == "3230")
        run(&RegTestSuite::test_3230, "Test-3230");
}

void RegTestSuite::test_3227(const std::string& test_case_name)
{
    list<string> not_dec_vars = gsi.get_not_declared_variables();
    if (not_dec_vars.size())
        complete_test_case(test_case_name, test_outcome::fail);
    else
        complete_test_case(test_case_name, test_outcome::pass);
}

void RegTestSuite::test_3228(const std::string& test_case_name)
{
    list<string> not_dec_vars = gsi.get_not_declared_variables();
    if (not_dec_vars.size())
        complete_test_case(test_case_name, test_outcome::fail);
    else
        complete_test_case(test_case_name, test_outcome::pass);
}

void RegTestSuite::test_3229(const std::string& test_case_name)
{
    list<string> not_reg_vars = gsi.get_not_registered_variables();
    if (not_reg_vars.size())
        complete_test_case(test_case_name, test_outcome::fail);
    else
        complete_test_case(test_case_name, test_outcome::pass);
}

void RegTestSuite::test_3230(const std::string& test_case_name)
{
    list<string> not_reg_vars = gsi.get_not_registered_variables();
    if (not_reg_vars.size())
        complete_test_case(test_case_name, test_outcome::fail);
    else
        complete_test_case(test_case_name, test_outcome::pass);
}

