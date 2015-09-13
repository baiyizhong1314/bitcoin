#include "omnicore/log.h"

#include "random.h"
#include "utiltime.h"

#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/rational.hpp>
#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <limits>
#include <string>

/**
 * Test to find bad numbers, which may trigger unexpected behavior of boost::multiprecision.
 *
 * Note: this is inefficient, and likely error-prone!
 */

typedef boost::rational<boost::multiprecision::checked_int128_t> rational_t;

// settings
static const int log_level = 0;
static const int64_t max_distance = 500;
static const int64_t max_rounds = 25;
static const int64_t max_total_seconds = 15; // seconds

// flags
static const unsigned int not_equal        = (1U << 0);
static const unsigned int equal            = (1U << 1);
static const unsigned int smaller          = (1U << 2);
static const unsigned int greater          = (1U << 3);
static const unsigned int smaller_or_equal = (1U << 4);
static const unsigned int greater_or_equal = (1U << 5);

// counters
static int64_t time_at_start = 0;
static int64_t total_calls = 0;
static int64_t seconds_so_far = 0;
static int64_t last_second = -1;
static bool dont_stop = true;


static int64_t get_random_int64()
{
    static uint64_t nMax = static_cast<int64_t>(std::numeric_limits<int64_t>::max());

    return static_cast<int64_t>(GetRand(nMax));
}

static rational_t get_random_rational()
{
    int64_t num = get_random_int64();
    int64_t denom = get_random_int64();

    while (num < 1) {
        num = get_random_int64();
    }

    while (denom < 1) {
        denom = get_random_int64();
    }

    return rational_t(num, denom);
}

static std::string bits_to_string(unsigned int bits)
{
    std::string result;

    result += ((bits & not_equal)        ? "1" : "0");
    result += ((bits & equal)            ? "1" : "0");
    result += ((bits & smaller)          ? "1" : "0");
    result += ((bits & greater)          ? "1" : "0");
    result += ((bits & smaller_or_equal) ? "1" : "0");
    result += ((bits & greater_or_equal) ? "1" : "0");

    return result;
}

template<typename NumberT>
static unsigned int get_bitmask(NumberT lhs, NumberT rhs)
{
    unsigned int bits = 0;

    if (lhs != rhs) {
        BOOST_REQUIRE(rhs != lhs);
        BOOST_REQUIRE(!(rhs == lhs));
        BOOST_REQUIRE((rhs < lhs) || (rhs > lhs));
        BOOST_REQUIRE((rhs > lhs) || (rhs < lhs));

        bits |= not_equal;
    }

    if (lhs == rhs) {
        BOOST_REQUIRE(rhs == lhs);
        BOOST_REQUIRE(!(rhs != lhs));
        BOOST_REQUIRE((rhs <= lhs) || (rhs >= lhs));
        BOOST_REQUIRE((rhs >= lhs) || (rhs <= lhs));
        BOOST_REQUIRE(!(lhs < rhs));
        BOOST_REQUIRE(!(lhs > rhs));
        BOOST_REQUIRE(!(rhs < lhs));
        BOOST_REQUIRE(!(rhs > lhs));

        bits |= equal;
    }

    if (lhs < rhs) {
        BOOST_REQUIRE(rhs >  lhs);
        BOOST_REQUIRE(rhs != lhs);
        BOOST_REQUIRE(lhs <= rhs);
        BOOST_REQUIRE(!(lhs == rhs));
        BOOST_REQUIRE(!(rhs == lhs));
        BOOST_REQUIRE(!(rhs <  lhs));

        bits |= smaller;
    }

    if (lhs > rhs) {
        BOOST_REQUIRE(rhs <  lhs);
        BOOST_REQUIRE(rhs != lhs);
        BOOST_REQUIRE(lhs >= rhs);
        BOOST_REQUIRE(!(lhs == rhs));
        BOOST_REQUIRE(!(rhs == lhs));
        BOOST_REQUIRE(!(rhs >  lhs));

        bits |= greater;
    }

    if (lhs <= rhs) {
        BOOST_REQUIRE(rhs >= lhs);
        BOOST_REQUIRE(lhs < rhs || lhs == rhs);
        BOOST_REQUIRE(rhs > lhs || rhs == lhs);

        bits |= smaller_or_equal;
    }

    if (lhs >= rhs) {
        BOOST_REQUIRE(rhs <= lhs);
        BOOST_REQUIRE(lhs > rhs || lhs == rhs);
        BOOST_REQUIRE(rhs < lhs || rhs == lhs);

        bits |= greater_or_equal;
    }


    return bits;
}

static std::string to_string(rational_t n)
{
    return strprintf("%23d / %23d", n.numerator(), n.denominator());
}

template <typename NumberT>
static bool is_overflow(NumberT a, NumberT b)
{
    return (((b > 0) && (a > (std::numeric_limits<NumberT>::max() - b))) ||
            ((b < 0) && (a < (std::numeric_limits<NumberT>::min() - b))));
}

static bool is_overflow_num(rational_t a, int64_t b)
{
    return is_overflow(a.numerator(), rational_t::int_type(b));
}

template<typename NumberT>
static void require_equal(unsigned int bits_reference, NumberT lhs, NumberT rhs, NumberT original_lhs, NumberT original_rhs)
{
    if (lhs <= 0) {
        if (log_level > 1) PrintToConsole("%s: returning because lhs <= 0!\n", __func__);
        return;
    }

    if (rhs <= 0) {
        if (log_level > 1) PrintToConsole("%s: returning because rhs <= 0!\n", __func__);
        return;
    }

    seconds_so_far = (GetTimeMillis() - time_at_start) / 1000;

    if ((log_level > 0) || (seconds_so_far != last_second)) {
        PrintToConsole("testing.. lhs: %s, rhs: %s, bits: %s.. %4d s.. rounds: %d\n",
                to_string(lhs), to_string(rhs), bits_to_string(bits_reference), seconds_so_far, total_calls);

        last_second = seconds_so_far;

        if (seconds_so_far > max_total_seconds) dont_stop = false;
    }

    unsigned int bits = get_bitmask(lhs, rhs);

    if (bits_reference != bits) {
        PrintToConsole("%s: ERROR: bits mismatch: %s != %s\n", __func__,
                bits_to_string(bits_reference), bits_to_string(bits));
        PrintToConsole("%s: ERROR: original_lhs: %s\n", __func__, to_string(original_lhs));
        PrintToConsole("%s: ERROR: original_rhs: %s\n", __func__, to_string(original_rhs));
        PrintToConsole("%s: ERROR: lhs:          %s\n", __func__, to_string(lhs));
        PrintToConsole("%s: ERROR: rhs:          %s\n", __func__, to_string(rhs));
    }

    BOOST_REQUIRE_EQUAL(bits_reference, bits);
    ++total_calls;
}

static bool add_to(const rational_t lhs, const rational_t rhs, const int64_t i, const int64_t j, rational_t& new_lhs, rational_t& new_rhs)
{
    if (is_overflow_num(lhs, i)) return false;
    if (is_overflow_num(rhs, j)) return false;

    if (i != 0) {
        if (lhs.numerator() + i == 0) return false;
        new_lhs = rational_t(lhs.numerator() + i, lhs.denominator());
    }

    if (j != 0) {
        if (rhs.numerator() + i == 0) return false;
        new_rhs = rational_t(rhs.numerator() + i, rhs.denominator());
    }

    return true;
}

template<typename NumberT>
static void test_it(NumberT lhs, NumberT rhs)
{
    if (lhs == 0) return;
    if (rhs == 0) return;

    unsigned int bits = get_bitmask(lhs, rhs);

    // equal
    for (int64_t i = -max_distance; i < max_distance; ++i) {
        if (is_overflow_num(lhs, i)) continue;
        if (is_overflow_num(rhs, i)) continue;

        NumberT new_lhs = lhs + i;
        NumberT new_rhs = rhs + i;

        require_equal(bits, new_lhs, new_rhs, lhs, rhs);
    }

    // lhs smaller than rhs strict
    if (bits & smaller && bits & not_equal) {
        BOOST_REQUIRE(!(bits & equal));
        BOOST_REQUIRE(!(bits & greater));

        // decrease lhs
        for (int64_t i = -max_distance; i < 0; ++i) {
            NumberT new_lhs, new_rhs;

            if (add_to(lhs, rhs, i, 0, new_lhs, new_rhs)) {
                // if (new_lhs == new_rhs) continue;
                require_equal(bits, new_lhs, new_rhs, lhs, rhs);
            }
        }

        // increase rhs
        for (int64_t j = 1; j < max_distance; ++j) {
            NumberT new_lhs, new_rhs;

            if (add_to(lhs, rhs, 0, j, new_lhs, new_rhs)) {
                // if (new_lhs == new_rhs) continue;
                require_equal(bits, new_lhs, new_rhs, lhs, rhs);
            }
        }
    }

    // lhs greater than rhs strict
    if (bits & greater && bits & not_equal) {
        BOOST_REQUIRE(!(bits & equal));
        BOOST_REQUIRE(!(bits & smaller));

        // decrease rhs
        for (int64_t j = -max_distance; j < 0; ++j) {
            NumberT new_lhs, new_rhs;

            if (add_to(lhs, rhs, 0, j, new_lhs, new_rhs)) {
                // if (new_lhs == new_rhs) continue;
                require_equal(bits, new_lhs, new_rhs, lhs, rhs);
            }
        }

        // increase lhs
        for (int64_t i = 1; i < max_distance; ++i) {
            NumberT new_lhs, new_rhs;

            if (add_to(lhs, rhs, i, 0, new_lhs, new_rhs)) {
                // if (new_lhs == new_rhs) continue;
                require_equal(bits, new_lhs, new_rhs, lhs, rhs);
            }
        }
    }
}

static void run_tests()
{
    // equal
    for (int64_t i = 0; i < max_rounds; ++i) {
        rational_t n = get_random_rational();
        rational_t m = n;

        test_it(n, m);
    }

    // likely not equal
    for (int64_t i = 0; i < max_rounds; ++i) {
        rational_t n = get_random_rational();
        rational_t m = get_random_rational();

        test_it(n, m);
    }

    // small on the left
    for (int64_t i = 0; i < max_rounds; ++i) {
        rational_t n = rational_t(i);
        rational_t m = get_random_rational();

        test_it(n, m);
    }

    // small on the right
    for (int64_t i = 0; i < max_rounds; ++i) {
        rational_t n = get_random_rational();
        rational_t m = rational_t(i);

        test_it(n, m);
    }

    // large on the left
    for (int64_t i = 0; i < max_rounds; ++i) {
        rational_t n = rational_t(std::numeric_limits<int64_t>::max() - i);
        rational_t m = get_random_rational();

        test_it(n, m);
    }

    // large on the right
    for (int64_t i = 0; i < max_rounds; ++i) {
        rational_t n = get_random_rational();
        rational_t m = rational_t(std::numeric_limits<int64_t>::max() - i);

        test_it(n, m);
    }
}

BOOST_AUTO_TEST_SUITE(omnicore_rational_comp_tests)

BOOST_AUTO_TEST_CASE(rational_comp_test_loop)
{
    time_at_start = GetTimeMillis();

    while (dont_stop) {
        run_tests();
    }
}

BOOST_AUTO_TEST_CASE(boost_i128)
{
    using boost::multiprecision::checked_int128_t;

    PrintToConsole("checked_int128_t:   max: %s\n",
            boost::lexical_cast<std::string>(std::numeric_limits<checked_int128_t>::max()));

    checked_int128_t num("131099371288072266900000000000000000000");    
    checked_int128_t denom("156352045478212887156");
    checked_int128_t result = num / denom;
    checked_int128_t expected("838488366986797800");

    PrintToConsole("checked_int128_t:   %s / %s = %s (expected: %s)\n",
            boost::lexical_cast<std::string>(num),
            boost::lexical_cast<std::string>(denom),
            boost::lexical_cast<std::string>(result),
            boost::lexical_cast<std::string>(expected));

    BOOST_CHECK_EQUAL(expected, result);
}

BOOST_AUTO_TEST_CASE(boost_u128)
{
    using boost::multiprecision::checked_uint128_t;

    PrintToConsole("checked_uint128_t:  max: %s\n",
            boost::lexical_cast<std::string>(std::numeric_limits<checked_uint128_t>::max()));

    checked_uint128_t num("131099371288072266900000000000000000000");    
    checked_uint128_t denom("156352045478212887156");
    checked_uint128_t result = num / denom;
    checked_uint128_t expected("838488366986797800");

    PrintToConsole("checked_uint128_t:  %s / %s = %s (expected: %s)\n",
            boost::lexical_cast<std::string>(num),
            boost::lexical_cast<std::string>(denom),
            boost::lexical_cast<std::string>(result),
            boost::lexical_cast<std::string>(expected));

    BOOST_CHECK_EQUAL(expected, result);
}

BOOST_AUTO_TEST_SUITE_END()
