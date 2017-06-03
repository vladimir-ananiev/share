#include "test_run_instance.hpp"

using namespace g2::fasth;

test_run_instance::test_run_instance()
    : d_state(test_run_state::not_yet), d_outcome(test_outcome::indeterminate)
{
}

test_run_instance::test_run_instance(test_run_state test_run_state, test_outcome test_outcome)
    : d_state(test_run_state), d_outcome(test_outcome)
{
}

test_run_instance::test_run_instance(const test_run_instance& src) {
    *this = src;
}

test_run_instance& test_run_instance::operator=(const test_run_instance& src) {
    tthread::lock_guard<tthread::fast_mutex> lg(d_mutex);
    d_state = src.d_state;
    d_outcome = src.d_outcome;
    return *this;
}
