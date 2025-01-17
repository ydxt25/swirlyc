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
#ifndef SWIRLY_ASH_COMPARE_HPP
#define SWIRLY_ASH_COMPARE_HPP

#include <type_traits>

/**
 * @addtogroup Util
 * @{
 */

namespace swirly {

template <typename ValueT>
class Comparable {
 public:
  friend constexpr bool operator==(const ValueT& lhs, const ValueT& rhs) noexcept
  {
    return lhs.compare(rhs) == 0;
  }

  friend constexpr bool operator!=(const ValueT& lhs, const ValueT& rhs) noexcept
  {
    return lhs.compare(rhs) != 0;
  }

  friend constexpr bool operator<(const ValueT& lhs, const ValueT& rhs) noexcept
  {
    return lhs.compare(rhs) < 0;
  }

  friend constexpr bool operator<=(const ValueT& lhs, const ValueT& rhs) noexcept
  {
    return lhs.compare(rhs) <= 0;
  }

  friend constexpr bool operator>(const ValueT& lhs, const ValueT& rhs) noexcept
  {
    return lhs.compare(rhs) > 0;
  }

  friend constexpr bool operator>=(const ValueT& lhs, const ValueT& rhs) noexcept
  {
    return lhs.compare(rhs) >= 0;
  }

 protected:
  ~Comparable() noexcept = default;
};

template <typename EnumT, typename std::enable_if_t<std::is_enum<EnumT>::value>* = nullptr>
constexpr int compare(EnumT lhs, EnumT rhs) noexcept
{
  int i{};
  if (lhs < rhs) {
    i = -1;
  } else if (lhs > rhs) {
    i = 1;
  } else {
    i = 0;
  }
  return i;
}

template <typename IntegralT,
          typename std::enable_if_t<std::is_integral<IntegralT>::value>* = nullptr>
constexpr int compare(IntegralT lhs, IntegralT rhs) noexcept
{
  int i{};
  if (lhs < rhs) {
    i = -1;
  } else if (lhs > rhs) {
    i = 1;
  } else {
    i = 0;
  }
  return i;
}

} // swirly

/** @} */

#endif // SWIRLY_ASH_COMPARE_HPP
