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
#ifndef SWIRLY_ELM_EXEC_HPP
#define SWIRLY_ELM_EXEC_HPP

#include <swirly/elm/Conv.hpp>
#include <swirly/elm/Request.hpp>

/**
 * @addtogroup Entity
 * @{
 */

namespace swirly {

/**
 * A transaction that occurs as an @ref Order transitions through a workflow.
 *
 * Trade executions represent the exchange of goods or services between counter-parties.
 */
class SWIRLY_API Exec : public Request {
 public:
  Exec(std::string_view trader, std::string_view market, std::string_view contr, Jday settlDay,
       Iden id, std::string_view ref, Iden orderId, State state, Side side, Lots lots, Ticks ticks,
       Lots resd, Lots exec, Cost cost, Lots lastLots, Ticks lastTicks, Lots minLots, Iden matchId,
       Role role, std::string_view cpty, Millis created) noexcept
    : Request{trader, market, contr, settlDay, id, ref, side, lots, created},
      orderId_{orderId},
      state_{state},
      ticks_{ticks},
      resd_{resd},
      exec_{exec},
      cost_{cost},
      lastLots_{lastLots},
      lastTicks_{lastTicks},
      minLots_{minLots},
      matchId_{matchId},
      role_{role},
      cpty_{cpty}
  {
  }

  ~Exec() noexcept override;

  // Copy.
  Exec(const Exec&) = delete;
  Exec& operator=(const Exec&) = delete;

  // Move.
  Exec(Exec&&);
  Exec& operator=(Exec&&) = delete;

  void toJson(std::ostream& os) const override;

  Iden orderId() const noexcept { return orderId_; }
  State state() const noexcept { return state_; }
  Ticks ticks() const noexcept { return ticks_; }
  Lots resd() const noexcept { return resd_; }
  Lots exec() const noexcept { return exec_; }
  Cost cost() const noexcept { return cost_; }
  Lots lastLots() const noexcept { return lastLots_; }
  Ticks lastTicks() const noexcept { return lastTicks_; }
  Lots minLots() const noexcept { return minLots_; }
  Iden matchId() const noexcept { return matchId_; }
  Role role() const noexcept { return role_; }
  std::string_view cpty() const noexcept { return +cpty_; }

  void revise(Lots lots) noexcept
  {
    using namespace enumops;
    state_ = State::Revise;
    const auto delta = lots_ - lots;
    assert(delta >= 0_lts);
    lots_ = lots;
    resd_ -= delta;
  }
  void cancel() noexcept
  {
    state_ = State::Cancel;
    resd_ = 0_lts;
  }
  void trade(Lots sumLots, Cost sumCost, Lots lastLots, Ticks lastTicks, Iden matchId, Role role,
             std::string_view cpty) noexcept;

  void trade(Lots lastLots, Ticks lastTicks, Iden matchId, Role role,
             std::string_view cpty) noexcept
  {
    trade(lastLots, swirly::cost(lastLots, lastTicks), lastLots, lastTicks, matchId, role, cpty);
  }

  boost::intrusive::set_member_hook<> idHook_;

 private:
  const Iden orderId_;
  State state_;
  const Ticks ticks_;
  /**
   * Must be greater than zero.
   */
  Lots resd_;
  /**
   * Must not be greater that lots.
   */
  Lots exec_;
  Cost cost_;
  Lots lastLots_;
  Ticks lastTicks_;
  /**
   * Minimum to be filled by this order.
   */
  const Lots minLots_;
  Iden matchId_;
  Role role_;
  Mnem cpty_;
};

using ExecIdSet = RequestIdSet<Exec>;

} // swirly

/** @} */

#endif // SWIRLY_ELM_EXEC_HPP
