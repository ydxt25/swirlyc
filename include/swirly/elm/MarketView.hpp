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
#ifndef SWIRLY_ELM_MARKETVIEW_HPP
#define SWIRLY_ELM_MARKETVIEW_HPP

#include <swirly/elm/MarketData.hpp>
#include <swirly/elm/Types.hpp>

#include <swirly/ash/Types.hpp>

/**
 * @addtogroup Entity
 * @{
 */

namespace swirly {

/**
 * A flattened view of a market.
 */
class SWIRLY_API MarketView {
 public:
  MarketView(std::string_view market, std::string_view contr, Jday settlDay, Lots lastLots,
             Ticks lastTicks, Millis lastTime, const MarketData& data) noexcept;

  MarketView(std::string_view market, std::string_view contr, Jday settlDay, Lots lastLots,
             Ticks lastTicks, Millis lastTime) noexcept;

  ~MarketView() noexcept;

  // Copy.
  MarketView(const MarketView&);
  MarketView& operator=(const MarketView&) = delete;

  // Move.
  MarketView(MarketView&&);
  MarketView& operator=(MarketView&&) = delete;

  auto market() const noexcept { return +market_; }
  auto contr() const noexcept { return +contr_; }
  auto settlDay() const noexcept { return settlDay_; }
  auto lastLots() const noexcept { return lastLots_; }
  auto lastTicks() const noexcept { return lastTicks_; }
  auto lastTime() const noexcept { return lastTime_; }

 private:
  const Mnem market_;
  const Mnem contr_;
  const Jday settlDay_;
  Lots lastLots_;
  Ticks lastTicks_;
  Millis lastTime_;
  const MarketData data_;
};

} // swirly

/** @} */

#endif // SWIRLY_ELM_MARKETVIEW_HPP
