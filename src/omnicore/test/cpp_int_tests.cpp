#include "omnicore/log.h"

#include "utilstrencodings.h"

#include <stdint.h>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/test/unit_test.hpp>

using boost::multiprecision::cpp_int;
using boost::multiprecision::int256_t;
using boost::multiprecision::checked_int128_t;
using boost::multiprecision::checked_int256_t;

BOOST_AUTO_TEST_SUITE(cpp_int_tests)

static void checkCalcFundraiser(int64_t amtTransfer, int64_t expectedValue)
{
    uint8_t bonusPerc = 0;
    uint8_t issuerPerc = 0;
    int64_t fundraiserSecs = 2;
    int64_t currentSecs = 1;
    int64_t numProps = 9223372036854775807;

    // Weeks in seconds
    checked_int128_t weeks_sec_ = 604800L;

    // Precision for all non-bitcoin values (bonus percentages, for example)
    checked_int128_t precision_ = 1000000000000L;

    // Precision for all percentages (10/100 = 10%)
    checked_int128_t percentage_precision = 100L;

    // Calculate the bonus seconds
    checked_int128_t bonusSeconds_ = fundraiserSecs - currentSecs;

    PrintToConsole(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    PrintToConsole("bonusSeconds_ = %s\n", boost::lexical_cast<std::string>(bonusSeconds_));

    // Calculate the whole number of weeks to apply bonus
    checked_int128_t weeks_ = (bonusSeconds_ / weeks_sec_) * precision_ + ((bonusSeconds_ % weeks_sec_) * precision_) / weeks_sec_;

    PrintToConsole("weeks_ = %s\n", boost::lexical_cast<std::string>(weeks_));

    // Calculate the earlybird percentage to be applied
    checked_int128_t ebPercentage_ = weeks_ * bonusPerc;

    PrintToConsole("ebPercentage_ = %s\n", boost::lexical_cast<std::string>(ebPercentage_));

    // Calcluate the bonus percentage to apply up to percentage_precision number of digits
    checked_int128_t bonusPercentage_ = (ebPercentage_ + (precision_ * percentage_precision)) / percentage_precision;

    PrintToConsole("bonusPercentage_ = %s\n", boost::lexical_cast<std::string>(bonusPercentage_));

    // Calculate the bonus percentage for the issuer
    checked_int128_t issuerPercentage_ = checked_int128_t(issuerPerc) * precision_ / percentage_precision;

    PrintToConsole("issuerPercentage_ = %s\n", boost::lexical_cast<std::string>(issuerPercentage_));

    // Precision for bitcoin amounts (satoshi)
    checked_int128_t satoshi_precision_ = 100000000;

    // Total tokens including remainders
    cpp_int createdTokens = cpp_int(amtTransfer) * cpp_int(numProps) * cpp_int(bonusPercentage_);
    cpp_int issuerTokens = (createdTokens / (satoshi_precision_ * precision_)) * (issuerPercentage_ / 100) * precision_;

    PrintToConsole("createdTokens = %s\n", boost::lexical_cast<std::string>(createdTokens));
    PrintToConsole("issuerTokens = %s\n", boost::lexical_cast<std::string>(issuerTokens));

    cpp_int createdTokens_int = createdTokens / (precision_ * satoshi_precision_);

    PrintToConsole("createdTokens_int = %s\n", boost::lexical_cast<std::string>(createdTokens_int));

    cpp_int createdTokens_int_wr = createdTokens / (cpp_int(precision_) * cpp_int(satoshi_precision_));
    PrintToConsole("createdTokens_int cpp_int all = %s\n", boost::lexical_cast<std::string>(createdTokens_int_wr));

    checked_int256_t createdTokens_int_256 = checked_int256_t(createdTokens) / (checked_int256_t(precision_) * checked_int256_t(satoshi_precision_));
    PrintToConsole("createdTokens_int 256_t = %s\n", boost::lexical_cast<std::string>(createdTokens_int_256));

    int64_t resultValue = createdTokens_int.convert_to<int64_t>();
    BOOST_CHECK_EQUAL(resultValue, expectedValue);
    PrintToConsole("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

BOOST_AUTO_TEST_CASE(satoshi_100_test)
{
    checkCalcFundraiser(        1, 92233720368);
    checkCalcFundraiser(       10, 922337203685);
    checkCalcFundraiser(      100, 9223372036854);
    checkCalcFundraiser(     1000, 92233720368547);
    checkCalcFundraiser(    10000, 922337203685477);
    checkCalcFundraiser(   100000, 9223372036854775);
    checkCalcFundraiser(  1000000, 92233720368547758);
    checkCalcFundraiser( 10000000, 922337203685477580);
    checkCalcFundraiser(100000000, 9223372036854775807);
}


BOOST_AUTO_TEST_SUITE_END()
