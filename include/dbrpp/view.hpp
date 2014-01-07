/*
 *  Copyright (C) 2013, 2014 Mark Aylett <mark.aylett@gmail.com>
 *
 *  This file is part of Doobry written by Mark Aylett.
 *
 *  Doobry is free software; you can redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Doobry is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 *  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with this program; if
 *  not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301 USA.
 */
#ifndef DBRPP_VIEW_HPP
#define DBRPP_VIEW_HPP

#include <dbrpp/rec.hpp>

namespace dbr {

/**
 * @addtogroup TypesView
 * @{
 */

class ViewRef {
    DbrView* impl_;
public:
    explicit
    ViewRef(DbrView& impl) noexcept
        : impl_{&impl}
    {
    }
    operator DbrView&() const noexcept
    {
        return *impl_;
    }
    DbrView*
    c_arg() const noexcept
    {
        return impl_;
    }
    ContrRecRef
    crec() const noexcept
    {
        return ContrRecRef{*impl_->contr.rec};
    }
    DbrDate
    settl_date() const noexcept
    {
        return impl_->settl_date;
    }
    DbrTicks
    bid_ticks(size_t level) const noexcept
    {
        return impl_->bid_ticks[level];
    }
    DbrLots
    bid_lots(size_t level) const noexcept
    {
        return impl_->bid_lots[level];
    }
    size_t
    bid_count(size_t level) const noexcept
    {
        return impl_->bid_count[level];
    }
    DbrTicks
    ask_ticks(size_t level) const noexcept
    {
        return impl_->ask_ticks[level];
    }
    DbrLots
    ask_lots(size_t level) const noexcept
    {
        return impl_->ask_lots[level];
    }
    size_t
    ask_count(size_t level) const noexcept
    {
        return impl_->ask_count[level];
    }
};

inline std::ostream&
operator <<(std::ostream& os, ViewRef view)
{
    return os << "crec=" << view.crec().mnem()
              << ",settl_date=" << view.settl_date()
              << ",bid_ticks=" << view.bid_ticks(0)
              << ",bid_lots=" << view.bid_lots(0)
              << ",bid_count=" << view.bid_count(0)
              << ",ask_ticks=" << view.ask_ticks(0)
              << ",ask_lots=" << view.ask_lots(0)
              << ",ask_count=" << view.ask_count(0);
}

/** @} */

} // dbr

#endif // DBRPP_VIEW_HPP
