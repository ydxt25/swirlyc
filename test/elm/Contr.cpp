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
#include <swirly/elm/Contr.hpp>

#include <swirly/tea/Test.hpp>

using namespace std;
using namespace swirly;

static_assert(sizeof(Contr) <= 4 * 64, "crossed cache-line boundary");

SWIRLY_TEST_CASE(ContrToString)
{
  Contr contr{"EURUSD", "EURUSD", "EUR", "USD", 1000000, 1, 1, 10000, 4, 1_lts, 10_lts};

  SWIRLY_CHECK(toString(contr) == //
               "{\"mnem\":\"EURUSD\""
               ",\"display\":\"EURUSD\""
               ",\"asset\":\"EUR\""
               ",\"ccy\":\"USD\""
               ",\"lotNumer\":1000000"
               ",\"lotDenom\":1"
               ",\"tickNumer\":1"
               ",\"tickDenom\":10000"
               ",\"pipDp\":4"
               ",\"minLots\":1"
               ",\"maxLots\":10"
               "}");
}
