/*
 * The Restful Matching-Engine.
 * Copyright (C) 2013, 2016 Swirly Cloud Limited.
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
#include <swirly/elm/Posn.hpp>

#include <swirly/ash/JulianDay.hpp>

#include <swirly/tea/Test.hpp>

using namespace std;
using namespace swirly;

static_assert(sizeof(Posn) <= 3 * 64, "crossed cache-line boundary");

SWIRLY_TEST_CASE(TraderPosnSet)
{
  constexpr auto settlDay = ymdToJd(2014, 2, 14);

  TraderPosnSet s;

  PosnPtr posn1{&*s.emplace("MARAYL", "EURUSD", settlDay, 0_lts, 0_cst, 0_lts, 0_cst)};
  SWIRLY_CHECK(posn1->refs() == 2);
  SWIRLY_CHECK(posn1->contr() == "EURUSD");
  SWIRLY_CHECK(posn1->settlDay() == settlDay);
  SWIRLY_CHECK(s.find("EURUSD", settlDay) != s.end());

  // Duplicate.
  PosnPtr posn2{&*s.emplace("MARAYL", "EURUSD", settlDay, 0_lts, 0_cst, 0_lts, 0_cst)};
  SWIRLY_CHECK(posn2->refs() == 3);
  SWIRLY_CHECK(posn2 == posn1);

  // Replace.
  PosnPtr posn3{&*s.emplaceOrReplace("MARAYL", "EURUSD", settlDay, 0_lts, 0_cst, 0_lts, 0_cst)};
  SWIRLY_CHECK(posn3->refs() == 2);
  SWIRLY_CHECK(posn3 != posn1);
  SWIRLY_CHECK(posn3->contr() == "EURUSD");
  SWIRLY_CHECK(posn3->settlDay() == settlDay);
}
