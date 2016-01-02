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
#include <swirly/fig/Serv.hpp>

#include <swirly/fig/TraderSess.hpp>

#include <swirly/elm/Exception.hpp>
#include <swirly/elm/MarketBook.hpp>
#include <swirly/elm/Model.hpp>

#include "ServFactory.hpp"
#include "TraderSessSet.hpp"

using namespace std;

namespace swirly {

struct Serv::Impl {
    Impl(const Model& model, Journ& journ, Millis now)
    : journ{journ}
    {
    }
    Journ& journ;
    detail::ServFactory factory;
    RecSet assets;
    RecSet contrs;
    RecSet markets;
    RecSet traders;
    detail::TraderSessSet emailIdx;
};

Serv::Serv(const Model& model, Journ& journ, Millis now)
:   impl_{make_unique<Impl>(model, journ, now)}
{
    impl_->assets = model.readAsset(impl_->factory);
    impl_->contrs = model.readContr(impl_->factory);
    impl_->markets = model.readMarket(impl_->factory);
    impl_->traders = model.readTrader(impl_->factory);

    for (auto& rec : impl_->traders) {
        auto& trader = static_cast<TraderSess&>(rec);
        impl_->emailIdx.insert(trader);
    }
}

Serv::~Serv() noexcept = default;

Serv::Serv(Serv&&) = default;

Serv& Serv::operator =(Serv&&) = default;

const RecSet& Serv::assets() const noexcept
{
    return impl_->assets;
}

const RecSet& Serv::contrs() const noexcept
{
    return impl_->contrs;
}

const RecSet& Serv::markets() const noexcept
{
    return impl_->markets;
}

const RecSet& Serv::traders() const noexcept
{
    return impl_->traders;
}

const MarketBook& Serv::createMarket(const StringView& mnem, const StringView& display,
                                     const StringView& contr, Jd settlDay, Jd expiryDay,
                                     MarketState state, Millis now)
{
    const auto& rec = impl_->markets.insert(impl_->factory.newMarket(mnem, display, contr, settlDay,
                                                                     expiryDay, state));
    const auto& market = static_cast<const MarketBook&>(rec);
    return market;
}

const MarketBook& Serv::updateMarket(const StringView& mnem, const StringView& display,
                                     MarketState state, Millis now)
{
    auto it = impl_->markets.find(mnem);
    if (it == impl_->markets.end())
        throwException<MarketNotFoundException>("market '%.*s' does not exist", SWIRLY_STR(mnem));
    auto& market = static_cast<MarketBook&>(*it);
    market.setDisplay(display);
    market.setState(state);
    return market;
}

const MarketBook& Serv::market(const StringView& mnem) const
{
    auto it = impl_->markets.find(mnem);
    if (it == impl_->markets.end())
        throwException<MarketNotFoundException>("market '%.*s' does not exist", SWIRLY_STR(mnem));
    const auto& market = static_cast<const MarketBook&>(*it);
    return market;
}

const TraderSess& Serv::createTrader(const StringView& mnem, const StringView& display,
                                     const StringView& email)
{
    Rec& rec = impl_->traders.insert(make_unique<TraderSess>(mnem, display, email));
    const auto& trader = static_cast<const TraderSess&>(rec);
    return trader;
}

const TraderSess& Serv::updateTrader(const StringView& mnem, const StringView& display)
{
    auto it = impl_->traders.find(mnem);
    if (it == impl_->traders.end())
        throwException<TraderNotFoundException>("trader '%.*s' does not exist", SWIRLY_STR(mnem));
    auto& trader = static_cast<TraderSess&>(*it);
    trader.setDisplay(display);
    return trader;
}

const TraderSess& Serv::trader(const StringView& mnem) const
{
    auto it = impl_->traders.find(mnem);
    if (it == impl_->traders.end())
        throwException<TraderNotFoundException>("trader '%.*s' does not exist", SWIRLY_STR(mnem));
    const auto& trader = static_cast<const TraderSess&>(*it);
    return trader;
}

const TraderSess* Serv::findTraderByEmail(const StringView& email) const
{
    const TraderSess* trader{nullptr};
    auto it = impl_->emailIdx.find(email);
    if (it != impl_->emailIdx.end())
        trader = &*it;
    return trader;
}

} // swirly
