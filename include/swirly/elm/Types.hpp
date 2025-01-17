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
#ifndef SWIRLY_ELM_TYPES_HPP
#define SWIRLY_ELM_TYPES_HPP

#include <swirly/ash/Enum.hpp>
#include <swirly/ash/String.hpp>

#include <boost/intrusive_ptr.hpp>

#include <cstdint>

/**
 * @addtogroup Domain
 * @{
 */

namespace swirly {

enum class Iden : int64_t {};

constexpr Iden operator""_id(unsigned long long val) noexcept
{
  return box<Iden>(val);
}

using Incs = int64_t;

enum class Lots : Incs {};

constexpr Lots operator""_lts(unsigned long long val) noexcept
{
  return box<Lots>(val);
}

/**
 * Unit representing the minimum price increment.
 */
enum class Ticks : Incs {};

constexpr Ticks operator""_tks(unsigned long long val) noexcept
{
  return box<Ticks>(val);
}

/**
 * Sum of lots and ticks.
 */
enum class Cost : Incs {};

constexpr Cost operator""_cst(unsigned long long val) noexcept
{
  return box<Cost>(val);
}

/**
 * Bitfield representing the state of a Market.
 */
using MarketState = unsigned;

/**
 * Maximum display characters.
 */
constexpr std::size_t DisplayMax{64};

/**
 * Maximum email characters.
 */
constexpr std::size_t EmailMax{64};

/**
 * Maximum mnemonic characters.
 */
constexpr std::size_t MnemMax{16};

/**
 * Maximum reference characters.
 */
constexpr std::size_t RefMax{64};

/**
 * Description suitable for display on user-interface.
 */
using Display = String<DisplayMax>;

/**
 * Email address.
 */
using Email = String<EmailMax>;

/**
 * Memorable identifier.
 */
using Mnem = String<MnemMax>;

/**
 * Reference.
 */
using Ref = String<RefMax>;

enum class AssetType { Commodity = 1, Corporate, Currency, Equity, Government, Index };

SWIRLY_API const char* enumString(AssetType type);

template <>
struct EnumTraits<AssetType> {
  static void print(std::ostream& os, AssetType val) noexcept { os << enumString(val); }
};

enum class Direct {
  /**
   * Aggressor bought. Taker lifted the offer resulting in a market uptick.
   */
  Paid = 1,
  /**
   * Aggressor sold. Taker hit the bid resulting in a market dow
   */
  Given = -1
};

SWIRLY_API const char* enumString(Direct direct);

template <>
struct EnumTraits<Direct> {
  static void print(std::ostream& os, Direct val) noexcept { os << enumString(val); }
};

enum class RecType {
  /**
   * Asset.
   */
  Asset = 1,
  /**
   * Contract.
   */
  Contr,
  /**
   * Market.
   */
  Market,
  /**
   * Trader.
   */
  Trader
};

SWIRLY_API const char* enumString(RecType type);

template <>
struct EnumTraits<RecType> {
  static void print(std::ostream& os, RecType val) noexcept { os << enumString(val); }
};

enum class Role {
  /**
   * No role.
   */
  None = 0,
  /**
   * Passive buyer or seller that receives the spread.
   */
  Maker,
  /**
   * Aggressive buyer or seller that crosses the market and pays the spread.
   */
  Taker
};

SWIRLY_API const char* enumString(Role role);

template <>
struct EnumTraits<Role> {
  static void print(std::ostream& os, Role val) noexcept { os << enumString(val); }
};

enum class Side { Buy = 1, Sell = -1 };

SWIRLY_API const char* enumString(Side side);

template <>
struct EnumTraits<Side> {
  static void print(std::ostream& os, Side val) noexcept { os << enumString(val); }
};

/**
 * Order states.
 * @image html OrderState.png
 */
enum class State {
  None = 0,
  /**
   * Initial state of a resting order placed in the order-book.
   */
  New,
  /**
   * State of a resting order that has been revised.
   */
  Revise,
  /**
   * State of a resting order that has been cancelled.
   */
  Cancel,
  /**
   * State of an order that has been partially or fully filled.
   */
  Trade
};

SWIRLY_API const char* enumString(State state);

template <>
struct EnumTraits<State> {
  static void print(std::ostream& os, State val) noexcept { os << enumString(val); }
};

} // swirly

/** @} */

/**
 * @addtogroup Entity
 * @{
 */

namespace swirly {

class Asset;
class Contr;
class Market;
class Trader;

class Request;
class Exec;
class Order;
class Posn;

using RequestPtr = boost::intrusive_ptr<Request>;
using ConstRequestPtr = boost::intrusive_ptr<const Request>;

using ExecPtr = boost::intrusive_ptr<Exec>;
using ConstExecPtr = boost::intrusive_ptr<const Exec>;

using OrderPtr = boost::intrusive_ptr<Order>;
using ConstOrderPtr = boost::intrusive_ptr<const Order>;

using PosnPtr = boost::intrusive_ptr<Posn>;
using ConstPosnPtr = boost::intrusive_ptr<const Posn>;

} // swirly

/** @} */

#endif // SWIRLY_ELM_TYPES_HPP
