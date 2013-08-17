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
#include "accnt.h"
#include "cache.h"
#include "exec.h"
#include "index.h"
#include "market.h"
#include "trader.h"

#include <dbr/ctx.h>
#include <dbr/err.h>
#include <dbr/util.h>

#include <stdbool.h>
#include <stdlib.h> // malloc()

struct DbrCtx_ {
    DbrPool pool;
    DbrModel model;
    struct FigCache cache;
    struct FigIndex index;
    struct FigExec exec;
};

static inline struct DbrRec*
get_id(DbrCtx ctx, int type, DbrIden id)
{
    struct DbrSlNode* node = fig_cache_find_rec_id(&ctx->cache, type, id);
    assert(node != fig_cache_end_rec(&ctx->cache));
    return dbr_rec_entry(node);
}

static DbrBool
emplace_recs(DbrCtx ctx, int type)
{
    struct DbrSlNode* node;
    ssize_t size = dbr_model_read_entity(ctx->model, type, &node);
    if (size == -1)
        return false;

    fig_cache_emplace_recs(&ctx->cache, type, node, size);
    return true;
}

static DbrBool
emplace_orders(DbrCtx ctx)
{
    struct DbrSlNode* node;
    if (dbr_model_read_entity(ctx->model, DBR_ORDER, &node) == -1)
        goto fail1;

    for (; node; node = node->next) {
        struct DbrOrder* order = dbr_order_entry(node);
        order->trader.rec = get_id(ctx, DBR_TRADER, order->trader.id);
        order->accnt.rec = get_id(ctx, DBR_ACCNT, order->accnt.id);
        order->market.rec = get_id(ctx, DBR_MARKET, order->market.id);

        struct FigMarket* market;
        if (!dbr_order_done(order)) {

            market = fig_market_lazy(order->market.rec, ctx->pool);
            if (dbr_unlikely(!market))
                goto fail2;

            if (dbr_unlikely(!fig_market_insert(market, order)))
                goto fail2;
        } else
            market = NULL;

        struct FigTrader* trader = fig_trader_lazy(order->trader.rec, ctx->pool, &ctx->index);
        if (dbr_unlikely(!trader)) {
            if (market)
                fig_market_remove(market, order);
            goto fail2;
        }

        // Transfer ownership.
        fig_trader_emplace_order(trader, order);
    }
    return true;
 fail2:
    // Free tail.
    do {
        struct DbrOrder* order = dbr_order_entry(node);
        node = node->next;
        dbr_pool_free_order(ctx->pool, order);
    } while (node);
 fail1:
    return false;
}

static DbrBool
emplace_membs(DbrCtx ctx)
{
    struct DbrSlNode* node;
    if (dbr_model_read_entity(ctx->model, DBR_MEMB, &node) == -1)
        goto fail1;

    for (; node; node = node->next) {
        struct DbrMemb* memb = dbr_memb_entry(node);
        memb->accnt.rec = get_id(ctx, DBR_ACCNT, memb->accnt.id);
        memb->trader.rec = get_id(ctx, DBR_TRADER, memb->trader.id);

        struct FigAccnt* accnt = fig_accnt_lazy(memb->accnt.rec, ctx->pool);
        if (dbr_unlikely(!accnt))
            goto fail2;

        // Transfer ownership.
        fig_accnt_emplace_memb(accnt, memb);
    }
    return true;
 fail2:
    // Free tail.
    do {
        struct DbrMemb* memb = dbr_memb_entry(node);
        node = node->next;
        dbr_pool_free_memb(ctx->pool, memb);
    } while (node);
 fail1:
    return false;
}

static DbrBool
emplace_trades(DbrCtx ctx)
{
    struct DbrSlNode* node;
    if (dbr_model_read_entity(ctx->model, DBR_TRADE, &node) == -1)
        goto fail1;

    for (; node; node = node->next) {
        struct DbrTrade* trade = dbr_trade_entry(node);
        trade->trader.rec = get_id(ctx, DBR_TRADER, trade->trader.id);
        trade->accnt.rec = get_id(ctx, DBR_ACCNT, trade->accnt.id);
        trade->market.rec = get_id(ctx, DBR_MARKET, trade->market.id);
        trade->cpty.rec = get_id(ctx, DBR_ACCNT, trade->cpty.id);

        struct FigAccnt* accnt = fig_accnt_lazy(trade->accnt.rec, ctx->pool);
        if (dbr_unlikely(!accnt))
            goto fail2;

        // Transfer ownership.
        fig_accnt_emplace_trade(accnt, trade);
    }
    return true;
 fail2:
    // Free tail.
    do {
        struct DbrTrade* trade = dbr_trade_entry(node);
        node = node->next;
        dbr_pool_free_trade(ctx->pool, trade);
    } while (node);
 fail1:
    return false;
}

static DbrBool
emplace_posns(DbrCtx ctx)
{
    struct DbrSlNode* node;
    if (dbr_model_read_entity(ctx->model, DBR_POSN, &node) == -1)
        goto fail1;

    for (; node; node = node->next) {
        struct DbrPosn* posn = dbr_posn_entry(node);
        posn->accnt.rec = get_id(ctx, DBR_ACCNT, posn->accnt.id);
        posn->instr.rec = get_id(ctx, DBR_INSTR, posn->instr.id);

        struct FigAccnt* accnt = fig_accnt_lazy(posn->accnt.rec, ctx->pool);
        if (dbr_unlikely(!accnt))
            goto fail2;

        // Transfer ownership.
        fig_accnt_emplace_posn(accnt, posn);
    }
    return true;
 fail2:
    // Free tail.
    do {
        struct DbrPosn* posn = dbr_posn_entry(node);
        node = node->next;
        dbr_pool_free_posn(ctx->pool, posn);
    } while (node);
 fail1:
    return false;
}

DBR_API DbrCtx
dbr_ctx_create(DbrPool pool, DbrJourn journ, DbrModel model)
{
    DbrCtx ctx = malloc(sizeof(struct DbrCtx_));
    if (dbr_unlikely(!ctx)) {
        dbr_err_set(DBR_ENOMEM, "out of memory");
        goto fail1;
    }

    ctx->pool = pool;
    ctx->model = model;
    fig_cache_init(&ctx->cache, pool);
    fig_index_init(&ctx->index);
    fig_exec_init(&ctx->exec, pool, journ, &ctx->index);

    // Data structures are fully initialised at this point.

    if (!emplace_recs(ctx, DBR_INSTR)
        || !emplace_recs(ctx, DBR_MARKET)
        || !emplace_recs(ctx, DBR_TRADER)
        || !emplace_recs(ctx, DBR_ACCNT)
        || !emplace_orders(ctx)
        || !emplace_membs(ctx)
        || !emplace_trades(ctx)
        || !emplace_posns(ctx)) {
        dbr_ctx_destroy(ctx);
        goto fail1;
    }

    return ctx;
 fail1:
    return NULL;
}

DBR_API void
dbr_ctx_destroy(DbrCtx ctx)
{
    if (ctx) {
        fig_cache_term(&ctx->cache);
        free(ctx);
    }
}

// Cache

DBR_API struct DbrSlNode*
dbr_ctx_first_rec(DbrCtx ctx, int type, size_t* size)
{
    return fig_cache_first_rec(&ctx->cache, type, size);
}

DBR_API struct DbrSlNode*
dbr_ctx_find_rec_id(DbrCtx ctx, int type, DbrIden id)
{
    return fig_cache_find_rec_id(&ctx->cache, type, id);
}

DBR_API struct DbrSlNode*
dbr_ctx_find_rec_mnem(DbrCtx ctx, int type, const char* mnem)
{
    return fig_cache_find_rec_mnem(&ctx->cache, type, mnem);
}

DBR_API struct DbrSlNode*
dbr_ctx_end_rec(DbrCtx ctx)
{
    return fig_cache_end_rec(&ctx->cache);
}

// Pool

DBR_API DbrMarket
dbr_ctx_market(DbrCtx ctx, struct DbrRec* mrec)
{
    return fig_market_lazy(mrec, ctx->pool);
}

DBR_API DbrTrader
dbr_ctx_trader(DbrCtx ctx, struct DbrRec* trec)
{
    return fig_trader_lazy(trec, ctx->pool, &ctx->index);
}

DBR_API DbrAccnt
dbr_ctx_accnt(DbrCtx ctx, struct DbrRec* arec)
{
    return fig_accnt_lazy(arec, ctx->pool);
}

// Exec

DBR_API struct DbrOrder*
dbr_ctx_submit(DbrCtx ctx, struct DbrRec* trec, struct DbrRec* arec, const char* ref,
               struct DbrRec* mrec, int action, DbrTicks ticks, DbrLots lots, DbrLots min,
               DbrFlags flags, struct DbrTrans* trans)
{
    const DbrMillis now = dbr_millis();
    return fig_exec_submit(&ctx->exec, trec, arec, ref, mrec, action, ticks, lots, min, flags,
                           now, trans);
}

DBR_API struct DbrOrder*
dbr_ctx_revise_id(DbrCtx ctx, DbrTrader trader, DbrIden id, DbrLots lots)
{
    const DbrMillis now = dbr_millis();
    return fig_exec_revise_id(&ctx->exec, trader, id, lots, now);
}

DBR_API struct DbrOrder*
dbr_ctx_revise_ref(DbrCtx ctx, DbrTrader trader, const char* ref, DbrLots lots)
{
    const DbrMillis now = dbr_millis();
    return fig_exec_revise_ref(&ctx->exec, trader, ref, lots, now);
}

DBR_API struct DbrOrder*
dbr_ctx_cancel_id(DbrCtx ctx, DbrTrader trader, DbrIden id)
{
    const DbrMillis now = dbr_millis();
    return fig_exec_cancel_id(&ctx->exec, trader, id, now);
}

DBR_API struct DbrOrder*
dbr_ctx_cancel_ref(DbrCtx ctx, DbrTrader trader, const char* ref)
{
    const DbrMillis now = dbr_millis();
    return fig_exec_cancel_ref(&ctx->exec, trader, ref, now);
}

DBR_API DbrBool
dbr_ctx_archive_order(DbrCtx ctx, DbrTrader trader, DbrIden id)
{
    const DbrMillis now = dbr_millis();
    return fig_exec_archive_order(&ctx->exec, trader, id, now);
}

DBR_API DbrBool
dbr_ctx_archive_trade(DbrCtx ctx, DbrAccnt accnt, DbrIden id)
{
    const DbrMillis now = dbr_millis();
    return fig_exec_archive_trade(&ctx->exec, accnt, id, now);
}

DBR_API void
dbr_ctx_free_matches(DbrCtx ctx, struct DbrSlNode* first)
{
    fig_exec_free_matches(&ctx->exec, first);
}
