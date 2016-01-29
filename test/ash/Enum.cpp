/*
 * Swirly Order-Book and Matching-Engine.
 * Copyright (C) 2013, 2015 Swirly Cloud Limited.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include <swirly/ash/Enum.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>

using namespace boost;
using namespace std;
using namespace swirly;

namespace swirly {

enum class Test : int { FOO = 1, BAR = 2, BAZ = 4, QUX = 8 };

constexpr Test operator""_test(unsigned long long val) noexcept
{
    return box<Test>(val);
}

const char* enumString(Test t)
{
    switch (t) {
        SWIRLY_ENUM_CASE(Test, FOO);
        SWIRLY_ENUM_CASE(Test, BAR);
        SWIRLY_ENUM_CASE(Test, BAZ);
        SWIRLY_ENUM_CASE(Test, QUX);
    }
    terminate();
}

template <>
struct EnumTraits<Test> {
    static void print(std::ostream& os, Test val) noexcept
    {
        os << enumString(val);
    }
};

} // swirly

BOOST_AUTO_TEST_SUITE(EnumSuite)

BOOST_AUTO_TEST_CASE(EnumOpsCase)
{
    using namespace enumops;

    // Addition assignment.
    {
        auto x = 5_test;
        BOOST_CHECK_EQUAL(x += 3_test, 8_test);
        BOOST_CHECK_EQUAL(x, 8_test);
    }
    // Subtraction assignment.
    {
        auto x = 5_test;
        BOOST_CHECK_EQUAL(x -= 3_test, 2_test);
        BOOST_CHECK_EQUAL(x, 2_test);
    }
    // Multiplication assignment.
    {
        auto x = 5_test;
        BOOST_CHECK_EQUAL(x *= 3_test, 15_test);
        BOOST_CHECK_EQUAL(x, 15_test);
    }
    // Division assignment.
    {
        auto x = 15_test;
        BOOST_CHECK_EQUAL(x /= 3_test, 5_test);
        BOOST_CHECK_EQUAL(x, 5_test);
    }
    // Modulo assignment.
    {
        auto x = 5_test;
        BOOST_CHECK_EQUAL(x %= 3_test, 2_test);
        BOOST_CHECK_EQUAL(x, 2_test);
    }
    // Bitwise AND assignment.
    {
        // 11100
        // 00111
        // 00100
        auto x = 28_test;
        BOOST_CHECK_EQUAL(x &= 7_test, 4_test);
        BOOST_CHECK_EQUAL(x, 4_test);
    }
    // Bitwise OR assignment.
    {
        // 11100
        // 00111
        // 11111
        auto x = 28_test;
        BOOST_CHECK_EQUAL(x |= 7_test, 31_test);
        BOOST_CHECK_EQUAL(x, 31_test);
    }
    // Bitwise XOR assignment.
    {
        // 11100
        // 00111
        // 11011
        auto x = 28_test;
        BOOST_CHECK_EQUAL(x ^= 7_test, 27_test);
        BOOST_CHECK_EQUAL(x, 27_test);
    }
    // Bitwise left shift assignment.
    {
        // 00111
        // 11100
        auto x = 7_test;
        BOOST_CHECK_EQUAL(x <<= 2_test, 28_test);
        BOOST_CHECK_EQUAL(x, 28_test);
    }
    // Bitwise right shift assignment.
    {
        // 11100
        // 00111
        auto x = 28_test;
        BOOST_CHECK_EQUAL(x >>= 2_test, 7_test);
        BOOST_CHECK_EQUAL(x, 7_test);
    }
    // Pre-increment.
    {
        auto x = 5_test;
        BOOST_CHECK_EQUAL(++x, 6_test);
        BOOST_CHECK_EQUAL(x, 6_test);
    }
    // Pre-decrement.
    {
        auto x = 5_test;
        BOOST_CHECK_EQUAL(--x, 4_test);
        BOOST_CHECK_EQUAL(x, 4_test);
    }
    // Post-increment.
    {
        auto x = 5_test;
        BOOST_CHECK_EQUAL(x++, 5_test);
        BOOST_CHECK_EQUAL(x, 6_test);
    }
    // Post-decrement.
    {
        auto x = 5_test;
        BOOST_CHECK_EQUAL(x--, 5_test);
        BOOST_CHECK_EQUAL(x, 4_test);
    }
    // Unary plus.
    {
        const auto x = 5_test;
        BOOST_CHECK_EQUAL(+x, 5_test);
    }
    // Unary minus.
    {
        const auto x = 5_test;
        BOOST_CHECK_EQUAL(-x, -5_test);
    }
    // Addition.
    {
        const auto x = 5_test;
        BOOST_CHECK_EQUAL(x + 3_test, 8_test);
    }
    // Subtraction.
    {
        const auto x = 5_test;
        BOOST_CHECK_EQUAL(x - 3_test, 2_test);
    }
    // Multiplication.
    {
        const auto x = 5_test;
        BOOST_CHECK_EQUAL(x * 3_test, 15_test);
    }
    // Division.
    {
        const auto x = 15_test;
        BOOST_CHECK_EQUAL(x / 3_test, 5_test);
    }
    // Modulo.
    {
        const auto x = 5_test;
        BOOST_CHECK_EQUAL(x % 3_test, 2_test);
    }
    // Bitwise NOT.
    {
        const auto x = box<Test>(~1);
        BOOST_CHECK_EQUAL(~x, 1_test);
    }
    // Bitwise AND.
    {
        // 11100
        // 00111
        // 00100
        const auto x = 28_test;
        BOOST_CHECK_EQUAL(x & 7_test, 4_test);
    }
    // Bitwise OR.
    {
        // 11100
        // 00111
        // 11111
        const auto x = 28_test;
        BOOST_CHECK_EQUAL(x | 7_test, 31_test);
    }
    // Bitwise XOR.
    {
        // 11100
        // 00111
        // 11011
        const auto x = 28_test;
        BOOST_CHECK_EQUAL(x ^ 7_test, 27_test);
    }
    // Bitwise left shift.
    {
        // 00111
        // 11100
        const auto x = 7_test;
        BOOST_CHECK_EQUAL(x << 2_test, 28_test);
    }
    // Bitwise right shift.
    {
        // 11100
        // 00111
        const auto x = 28_test;
        BOOST_CHECK_EQUAL(x >> 2_test, 7_test);
    }
}

BOOST_AUTO_TEST_CASE(EnumStringCase)
{
    BOOST_CHECK_EQUAL(lexical_cast<string>(Test::FOO), "FOO");
    BOOST_CHECK_EQUAL(lexical_cast<string>(Test::BAR), "BAR");
    BOOST_CHECK_EQUAL(lexical_cast<string>(Test::BAZ), "BAZ");
    BOOST_CHECK_EQUAL(lexical_cast<string>(Test::QUX), "QUX");
}

BOOST_AUTO_TEST_SUITE_END()
