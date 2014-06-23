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
#ifndef DBR_FIG_CTX_H
#define DBR_FIG_CTX_H

#include <dbr/fig/async.h>
#include <dbr/fig/handler.h>

#include <dbr/elm/types.h>

/**
 * @addtogroup Ctx
 * @{
 */

typedef struct FigCtx* DbrCtx;

DBR_API DbrCtx
dbr_ctx_create(const char* mdaddr, const char* traddr, DbrMillis tmout, size_t capacity,
               DbrHandler handler);

DBR_API void
dbr_ctx_destroy(DbrCtx ctx);

DBR_API DbrAsync
dbr_ctx_async(DbrCtx ctx);

DBR_API const unsigned char*
dbr_ctx_uuid(DbrCtx ctx);

/** @} */

#endif // DBR_FIG_CTX_H