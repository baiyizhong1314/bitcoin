#include "omnicore/omnicore.h"

#include <stdint.h>

#include <boost/test/unit_test.hpp>

extern int64_t GetDevOmni(unsigned int nTime);

BOOST_AUTO_TEST_SUITE(omnicore_reward_tests)

BOOST_AUTO_TEST_CASE(exodus_reward)
{
    BOOST_CHECK_EQUAL(507030922981, GetDevOmni(1382289200));
    BOOST_CHECK_EQUAL(507097784113, GetDevOmni(1382289794));
}


BOOST_AUTO_TEST_SUITE_END()
