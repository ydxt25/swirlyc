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

using namespace std;

namespace swirly {

Posn::~Posn() noexcept = default;

Posn::Posn(Posn&&) = default;

void Posn::toJson(ostream& os) const
{
  os << "{\"trader\":\"" << trader_ //
     << "\",\"contr\":\"" << contr_ //
     << "\",\"settlDate\":";
  if (settlDay_ != 0_jd) {
    os << jdToIso(settlDay_);
  } else {
    os << "null";
  }
  if (buyLots_ != 0_lts) {
    os << ",\"buyLots\":" << buyLots_ //
       << ",\"buyCost\":" << buyCost_;
  } else {
    os << ",\"buyLots\":0,\"buyCost\":0";
  }
  if (sellLots_ != 0_lts) {
    os << ",\"sellLots\":" << sellLots_ //
       << ",\"sellCost\":" << sellCost_;
  } else {
    os << ",\"sellLots\":0,\"sellCost\":0";
  }
  os << '}';
}

TraderPosnSet::~TraderPosnSet() noexcept
{
  set_.clear_and_dispose([](Posn* ptr) { ptr->release(); });
}

TraderPosnSet::TraderPosnSet(TraderPosnSet&&) = default;

TraderPosnSet& TraderPosnSet::operator=(TraderPosnSet&&) = default;

TraderPosnSet::Iterator TraderPosnSet::insert(const ValuePtr& value) noexcept
{
  Iterator it;
  bool inserted;
  std::tie(it, inserted) = set_.insert(*value);
  if (inserted) {
    // Take ownership if inserted.
    value->addRef();
  }
  return it;
}

TraderPosnSet::Iterator TraderPosnSet::insertHint(ConstIterator hint,
                                                  const ValuePtr& value) noexcept
{
  auto it = set_.insert(hint, *value);
  // Take ownership.
  value->addRef();
  return it;
}

TraderPosnSet::Iterator TraderPosnSet::insertOrReplace(const ValuePtr& value) noexcept
{
  Iterator it;
  bool inserted;
  std::tie(it, inserted) = set_.insert(*value);
  if (!inserted) {
    // Replace if exists.
    ValuePtr prev{&*it, false};
    set_.replace_node(it, *value);
    it = Set::s_iterator_to(*value);
  }
  // Take ownership.
  value->addRef();
  return it;
}

} // swirly
