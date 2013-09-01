/*
 *  Copyright (C) 2013 Mark Aylett <mark.aylett@gmail.com>
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
#ifndef FIG_MATCH_H
#define FIG_MATCH_H

#include <dbr/defs.h>
#include <dbr/pool.h>
#include <dbr/types.h>

// Used by matching engine to build a list of matches.

// Match taker with list of maker orders.

DBR_EXTERN DbrBool
fig_match_orders(DbrJourn journ, struct DbrBook* book, struct DbrOrder* taker,
                 struct DbrTrans* trans, DbrPool pool);

#endif // FIG_MATCH_H
