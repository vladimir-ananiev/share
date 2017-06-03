#pragma once
#ifndef INC_LIBG2FASTH_JUNIT_REPORT_H
#define INC_LIBG2FASTH_JUNIT_REPORT_H

#include "tinyxml2.hpp"
#include "xml_serialization.hpp"

namespace g2 {
namespace fasth {
/**
* This class holds information about failure of test case
* such as failure type and failure reason.
*/
class testcase_failure_data : public xmls::serializable {
public:
    testcase_failure_data() {
        set_class_name("failure");
        register_type("type", &d_type);
    }
    void set_properties(std::string failure_type, std::string failure_reason) {
        d_type = failure_type;
        set_inner_text(failure_reason);
    }
private:
    xmls::x_string d_type;
};

/**
* This class holds information about test case such as its
* name, classname and failure occured, if it has failed.
*/
class testcase_data : public xmls::serializable {
public:
    testcase_data() {
        set_class_name("testcase");
        register_type("classname", &d_classname);
        register_type("name", &d_name);
        register_type("failure", &d_failure);
    }
    void fail_test_case(std::string failure_type, std::string failure_reason) {
        d_failure.new_element()->set_properties(failure_type, failure_reason);
    }
    void set_properties(std::string class_name, std::string name) {
        d_classname = class_name;
        d_name = name;
    }
private:
    xmls::x_string d_classname;
    xmls::x_string d_name;
    xmls::collection<testcase_failure_data> d_failure;
};

/**
* This class holds information about test suite such as suite name,
* number of tests it is containing and collection of test cases.
*/
class testsuite_data : public xmls::serializable {
public:
    testsuite_data(xmls::x_int tests) {
        set_class_name("testsuite");
        register_type("tests", &d_tests);
        register_type("testcases", &d_testcases);
        d_tests = tests;
    }
    testcase_data * add_testcase(std::string class_name, std::string name) {
        auto test_case = d_testcases.new_element();
        test_case->set_properties(class_name, name);
        return test_case;
    }
private:
    xmls::collection<testcase_data> d_testcases;
    xmls::x_int d_tests;
};
}
}

#endif // INC_LIBG2FASTH_JUNIT_REPORT_H