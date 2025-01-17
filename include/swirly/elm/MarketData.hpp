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
#ifndef SWIRLY_ELM_MARKETDATA_HPP
#define SWIRLY_ELM_MARKETDATA_HPP

#include <swirly/ash/Defs.hpp>

/**
 * @addtogroup Domain
 * @{
 */

namespace swirly {

class SWIRLY_API MarketData {
 public:
  MarketData() noexcept {}
  ~MarketData() noexcept;

  // Copy.
  MarketData(const MarketData&) = default;
  MarketData& operator=(const MarketData&) = default;

  // Move.
  MarketData(MarketData&&) = default;
  MarketData& operator=(MarketData&&) = default;
};

} // swirly

/** @} */

#endif // SWIRLY_ELM_MARKETDATA_HPP
