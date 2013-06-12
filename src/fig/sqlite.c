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
#define _XOPEN_SOURCE 700 // strnlen()
#include "sqlite.h"

#include "sqlite3.h"

#include <dbr/conv.h>

#include <elm/ctx.h>
#include <elm/err.h>

#include <ash/queue.h>

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#define INSERT_ORDER_SQL                                                \
    "INSERT INTO order_ (id, rev, status, trader, accnt, ref,"          \
    " market, action, ticks, resd, exec, lots, min, flags,"             \
    " archive, created, modified)"                                      \
    " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0, ?, ?)"

#define UPDATE_ORDER_SQL                                                \
    "UPDATE order_ SET rev = ?, status = ?, resd = ?, exec = ?,"        \
    " lots = ?,  modified = ?"                                          \
    " WHERE id = ?"

#define ARCHIVE_ORDER_SQL                                             \
    "UPDATE order_ SET archive = 1, modified = ?"                     \
    " WHERE id = ?"

#define INSERT_TRADE_SQL                                                \
    "INSERT INTO trade (id, match, order_, order_rev, cpty, role,"      \
    " action, ticks, resd, exec, lots, settl_date, archive, created,"   \
    " modified)"                                                        \
    " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0, ?, ?)"

#define ARCHIVE_TRADE_SQL                                               \
    "UPDATE trade SET archive = 1, modified = ?"                        \
    " WHERE id = ?"

#define SELECT_INSTR_SQL                                                \
    "SELECT id, mnem, display, asset_type, instr_type, asset, ccy,"     \
    " tick_numer, tick_denom, lot_numer, lot_denom, pip_dp, min_lots,"  \
    " max_lots"                                                         \
    " FROM instr_v"

#define SELECT_MARKET_SQL                            \
    "SELECT id, mnem, instr, tenor, settl_date"      \
    " FROM market_v"

#define SELECT_TRADER_SQL                                    \
    "SELECT id, mnem, display, email"                        \
    " FROM trader_v"

#define SELECT_ACCNT_SQL                                      \
    "SELECT id, mnem, display, email"                         \
    " FROM accnt_v"

#define SELECT_ORDER_SQL                                                \
    "SELECT id, rev, status, trader, accnt, ref, market, action,"       \
    " ticks, resd, exec, lots, min, flags, created, modified"           \
    " FROM order_ WHERE archive = 0 ORDER BY id"

#define SELECT_MEMB_SQL                                    \
    "SELECT accnt, trader"                                 \
    " FROM memb ORDER BY accnt"

#define SELECT_TRADE_SQL                                                \
    "SELECT id, match, order_, order_rev, trader, accnt, ref, market,"  \
    " cpty, role, action, ticks, resd, exec, lots, settl_date,"         \
    " created, modified"                                                \
    " FROM trade_v WHERE archive = 0 ORDER BY id"

#define SELECT_POSN_SQL                                         \
    "SELECT id, accnt, instr, settl_date, action, licks, lots"  \
    " FROM posn_v ORDER BY accnt, instr, settl_date, action"

// Only called if failure occurs during cache load, so no need to free state members as they will
// not have been allocated.

static inline void
free_recs(struct ElmCtx* ctx, struct AshQueue* rq)
{
    struct DbrSlNode* node = ash_queue_first(rq);
    while (node) {
        struct DbrRec* rec = dbr_rec_entry(node);
        node = node->next;
        elm_ctx_free_rec(ctx, rec);
    }
}

static inline void
free_orders(struct ElmCtx* ctx, struct AshQueue* oq)
{
    struct DbrSlNode* node = ash_queue_first(oq);
    while (node) {
        struct DbrOrder* order = dbr_order_entry(node);
        node = node->next;
        elm_ctx_free_order(ctx, order);
    }
}

static inline void
free_trades(struct ElmCtx* ctx, struct AshQueue* tq)
{
    struct DbrSlNode* node = ash_queue_first(tq);
    while (node) {
        struct DbrTrade* trade = dbr_trade_entry(node);
        node = node->next;
        elm_ctx_free_trade(ctx, trade);
    }
}

static inline void
free_posns(struct ElmCtx* ctx, struct AshQueue* pq)
{
    struct DbrSlNode* node = ash_queue_first(pq);
    while (node) {
        struct DbrPosn* posn = dbr_posn_entry(node);
        node = node->next;
        elm_ctx_free_posn(ctx, posn);
    }
}

static sqlite3_stmt*
prepare(sqlite3* db, const char* sql)
{
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare(db, sql, -1, &stmt, NULL);
    if (dbr_unlikely(rc != SQLITE_OK)) {
        elm_err_set(DBR_EDBSQL, sqlite3_errmsg(db));
        stmt = NULL;
    }
    return stmt;
}

static DbrBool
exec_sql(sqlite3* db, const char* sql)
{
    sqlite3_stmt* stmt = prepare(db, sql);
    if (!stmt)
        goto fail1;

    // SQLITE_ROW  100
    // SQLITE_DONE 101

    if (dbr_unlikely(sqlite3_step(stmt) < SQLITE_ROW)) {
        elm_err_set(DBR_EDBSQL, sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        goto fail1;
    }

    sqlite3_finalize(stmt);
    return true;
 fail1:
    return false;
}

static DbrBool
exec_stmt(sqlite3* db, sqlite3_stmt* stmt)
{
    int rc = sqlite3_step(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    if (dbr_unlikely(rc != SQLITE_DONE)) {
        elm_err_set(DBR_EDBSQL, sqlite3_errmsg(db));
        return false;
    }
    return true;
}

#if DBR_DEBUG_LEVEL >= 2
static void
trace_sql(void* unused, const char* sql)
{
    DBR_DEBUG1F("%s", sql);
}
#endif // DBR_DEBUG_LEVEL >= 2

static inline int
bind_text(sqlite3_stmt* stmt, int col, const char* text, size_t maxlen)
{
    return sqlite3_bind_text(stmt, col, text, strnlen(text, maxlen), SQLITE_STATIC);
}

static DbrBool
bind_insert_order(struct FigSqlite* sqlite, const struct DbrOrder* order)
{
    enum {
        ID = 1,
        REV,
        STATUS,
        TRADER,
        ACCNT,
        REF,
        MARKET,
        ACTION,
        TICKS,
        RESD,
        EXEC,
        LOTS,
        MIN,
        FLAGS,
        CREATED,
        MODIFIED
    };
    sqlite3_stmt* stmt = sqlite->insert_order;
    int rc = sqlite3_bind_int64(stmt, ID, order->id);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int(stmt, REV, order->rev);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int(stmt, STATUS, order->status);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, TRADER, order->trader.rec->id);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, ACCNT, order->accnt.rec->id);
    if (rc != SQLITE_OK)
        goto fail1;

    if (order->ref[0] != '\0')
        rc = bind_text(stmt, REF, order->ref, DBR_REF_MAX);
    else
        rc = sqlite3_bind_null(stmt, REF);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, MARKET, order->market.rec->id);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int(stmt, ACTION, order->action);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, TICKS, order->ticks);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, RESD, order->resd);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, EXEC, order->exec);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, LOTS, order->lots);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, MIN, order->min);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, FLAGS, order->flags);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, CREATED, order->created);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, MODIFIED, order->modified);
    if (rc != SQLITE_OK)
        goto fail1;

    return true;
 fail1:
    elm_err_set(DBR_EDBSQL, sqlite3_errmsg(sqlite->db));
    sqlite3_clear_bindings(stmt);
    return false;
}

static DbrBool
bind_update_order(struct FigSqlite* sqlite, DbrIden id, int rev, int status,
                  DbrLots resd, DbrLots exec, DbrLots lots, DbrMillis now)
{
    enum {
        REV = 1,
        STATUS,
        RESD,
        EXEC,
        LOTS,
        MODIFIED,
        ID
    };
    sqlite3_stmt* stmt = sqlite->update_order;
    int rc = sqlite3_bind_int(stmt, REV, rev);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int(stmt, STATUS, status);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, RESD, resd);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, EXEC, exec);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, LOTS, lots);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, MODIFIED, now);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, ID, id);
    if (rc != SQLITE_OK)
        goto fail1;

    return true;
 fail1:
    elm_err_set(DBR_EDBSQL, sqlite3_errmsg(sqlite->db));
    sqlite3_clear_bindings(stmt);
    return false;
}

static DbrBool
bind_archive_order(struct FigSqlite* sqlite, DbrIden id, DbrMillis now)
{
    enum {
        MODIFIED = 1,
        ID
    };
    sqlite3_stmt* stmt = sqlite->archive_order;
    int rc = sqlite3_bind_int64(stmt, MODIFIED, now);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, ID, id);
    if (rc != SQLITE_OK)
        goto fail1;

    return true;
 fail1:
    elm_err_set(DBR_EDBSQL, sqlite3_errmsg(sqlite->db));
    sqlite3_clear_bindings(stmt);
    return false;
}

static DbrBool
bind_insert_trade(struct FigSqlite* sqlite, const struct DbrTrade* trade)
{
    enum {
        ID = 1,
        MATCH,
        ORDER,
        ORDER_REV,
        CPTY,
        ROLE,
        ACTION,
        TICKS,
        RESD,
        EXEC,
        LOTS,
        SETTL_DATE,
        CREATED,
        MODIFIED
    };
    sqlite3_stmt* stmt = sqlite->insert_trade;
    int rc = sqlite3_bind_int64(stmt, ID, trade->id);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, MATCH, trade->match);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, ORDER, trade->order);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int(stmt, ORDER_REV, trade->order_rev);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, CPTY, trade->cpty.rec->id);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int(stmt, ROLE, trade->role);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int(stmt, ACTION, trade->action);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, TICKS, trade->ticks);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, RESD, trade->resd);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, EXEC, trade->exec);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, LOTS, trade->lots);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int(stmt, SETTL_DATE, trade->settl_date);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, CREATED, trade->created);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, MODIFIED, trade->modified);
    if (rc != SQLITE_OK)
        goto fail1;

    return true;
 fail1:
    elm_err_set(DBR_EDBSQL, sqlite3_errmsg(sqlite->db));
    sqlite3_clear_bindings(stmt);
    return false;
}

static DbrBool
bind_archive_trade(struct FigSqlite* sqlite, DbrIden id, DbrMillis now)
{
    enum {
        MODIFIED = 1,
        ID
    };
    sqlite3_stmt* stmt = sqlite->archive_trade;
    int rc = sqlite3_bind_int64(stmt, MODIFIED, now);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, ID, id);
    if (rc != SQLITE_OK)
        goto fail1;

    return true;
 fail1:
    elm_err_set(DBR_EDBSQL, sqlite3_errmsg(sqlite->db));
    sqlite3_clear_bindings(stmt);
    return false;
}

static ssize_t
select_instr(struct FigSqlite* sqlite, struct DbrSlNode** first)
{
    enum {
        ID,
        MNEM,
        DISPLAY,
        ASSET_TYPE,
        INSTR_TYPE,
        ASSET,
        CCY,
        TICK_NUMER,
        TICK_DENOM,
        LOT_NUMER,
        LOT_DENOM,
        PIP_DP,
        MIN_LOTS,
        MAX_LOTS
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_INSTR_SQL);
    if (!stmt)
        goto fail1;

    struct AshQueue rq;
    ash_queue_init(&rq);

    ssize_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            struct DbrRec* rec = elm_ctx_alloc_rec(sqlite->ctx);
            if (!rec)
                goto fail2;

            // Header.

            rec->type = DBR_INSTR;
            rec->id = sqlite3_column_int64(stmt, ID);
            strncpy(rec->mnem,
                    (const char*)sqlite3_column_text(stmt, MNEM), DBR_MNEM_MAX);

            // Body.

            strncpy(rec->instr.display,
                    (const char*)sqlite3_column_text(stmt, DISPLAY), DBR_DISPLAY_MAX);
            strncpy(rec->instr.asset_type,
                    (const char*)sqlite3_column_text(stmt, ASSET_TYPE), DBR_MNEM_MAX);
            strncpy(rec->instr.instr_type,
                    (const char*)sqlite3_column_text(stmt, INSTR_TYPE), DBR_MNEM_MAX);
            strncpy(rec->instr.asset,
                    (const char*)sqlite3_column_text(stmt, ASSET), DBR_MNEM_MAX);
            strncpy(rec->instr.ccy,
                    (const char*)sqlite3_column_text(stmt, CCY), DBR_MNEM_MAX);

            const int tick_numer = sqlite3_column_int(stmt, TICK_NUMER);
            const int tick_denom = sqlite3_column_int(stmt, TICK_DENOM);
            const double price_inc = dbr_fract_to_real(tick_numer, tick_denom);
            rec->instr.tick_numer = tick_numer;
            rec->instr.tick_denom = tick_denom;
            rec->instr.price_inc = price_inc;

            const int lot_numer = sqlite3_column_int(stmt, LOT_NUMER);
            const int lot_denom = sqlite3_column_int(stmt, LOT_DENOM);
            const double qty_inc = dbr_fract_to_real(lot_numer, lot_denom);
            rec->instr.lot_numer = lot_numer;
            rec->instr.lot_denom = lot_denom;
            rec->instr.qty_inc = qty_inc;

            rec->instr.price_dp = dbr_real_to_dp(price_inc);
            rec->instr.pip_dp = sqlite3_column_int(stmt, PIP_DP);
            rec->instr.qty_dp = dbr_real_to_dp(qty_inc);

            rec->instr.min_lots = sqlite3_column_int64(stmt, MIN_LOTS);
            rec->instr.max_lots = sqlite3_column_int64(stmt, MAX_LOTS);
            rec->instr.state = NULL;

            DBR_DEBUG3F("instr: id=%ld,mnem=%.16s,display=%.64s,asset_type=%.16s,instr_type=%.16s,"
                        "asset=%.16s,ccy=%.16s,price_inc=%f,qty_inc=%.2f,price_dp=%d,pip_dp=%d,"
                        "qty_dp=%d,min_lots=%ld,max_lots=%ld",
                        rec->id, rec->mnem, rec->instr.display, rec->instr.asset_type,
                        rec->instr.instr_type, rec->instr.asset, rec->instr.ccy,
                        rec->instr.price_inc, rec->instr.qty_inc, rec->instr.price_dp,
                        rec->instr.pip_dp, rec->instr.qty_dp, rec->instr.min_lots,
                        rec->instr.max_lots);

            ash_queue_push(&rq, &rec->model_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            elm_err_set(DBR_EDBSQL, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = rq.first;
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    free_recs(sqlite->ctx, &rq);
    *first = NULL;
 fail1:
    return -1;
}

static ssize_t
select_market(struct FigSqlite* sqlite, struct DbrSlNode** first)
{
    enum {
        ID,
        MNEM,
        INSTR,
        TENOR,
        SETTL_DATE
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_MARKET_SQL);
    if (!stmt)
        goto fail1;

    struct AshQueue rq;
    ash_queue_init(&rq);

    ssize_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            struct DbrRec* rec = elm_ctx_alloc_rec(sqlite->ctx);
            if (!rec)
                goto fail2;

            // Header.

            rec->type = DBR_MARKET;
            rec->id = sqlite3_column_int64(stmt, ID);
            strncpy(rec->mnem,
                    (const char*)sqlite3_column_text(stmt, MNEM), DBR_MNEM_MAX);

            // Body.

            rec->market.instr.id = sqlite3_column_int64(stmt, INSTR);
            strncpy(rec->market.tenor,
                    (const char*)sqlite3_column_text(stmt, TENOR), DBR_TENOR_MAX);
            rec->market.settl_date = sqlite3_column_int(stmt, SETTL_DATE);
            rec->market.state = NULL;

            DBR_DEBUG3F("market: id=%ld,mnem=%.16s,instr=%ld,tenor=%.8s",
                        rec->id, rec->mnem, rec->market.instr.id, rec->market.tenor);

            ash_queue_push(&rq, &rec->model_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            elm_err_set(DBR_EDBSQL, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = rq.first;
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    free_recs(sqlite->ctx, &rq);
    *first = NULL;
 fail1:
    return -1;
}

static ssize_t
select_trader(struct FigSqlite* sqlite, struct DbrSlNode** first)
{
    enum {
        ID,
        MNEM,
        DISPLAY,
        EMAIL
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_TRADER_SQL);
    if (!stmt)
        goto fail1;

    struct AshQueue rq;
    ash_queue_init(&rq);

    size_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            struct DbrRec* rec = elm_ctx_alloc_rec(sqlite->ctx);
            if (!rec)
                goto fail2;

            // Header.

            rec->type = DBR_TRADER;
            rec->id = sqlite3_column_int64(stmt, ID);
            strncpy(rec->mnem,
                    (const char*)sqlite3_column_text(stmt, MNEM), DBR_MNEM_MAX);

            // Body.

            strncpy(rec->trader.display,
                    (const char*)sqlite3_column_text(stmt, DISPLAY), DBR_DISPLAY_MAX);
            strncpy(rec->trader.email,
                    (const char*)sqlite3_column_text(stmt, EMAIL), DBR_EMAIL_MAX);
            rec->trader.state = NULL;

            DBR_DEBUG3F("trader: id=%ld,mnem=%.16s,display=%.64s,email=%.64s",
                       rec->id, rec->mnem, rec->trader.display, rec->trader.email);

            ash_queue_push(&rq, &rec->model_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            elm_err_set(DBR_EDBSQL, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = rq.first;
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    free_recs(sqlite->ctx, &rq);
    *first = NULL;
 fail1:
    return -1;
}

static ssize_t
select_accnt(struct FigSqlite* sqlite, struct DbrSlNode** first)
{
    enum {
        ID,
        MNEM,
        DISPLAY,
        EMAIL
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_ACCNT_SQL);
    if (!stmt)
        goto fail1;

    struct AshQueue rq;
    ash_queue_init(&rq);

    size_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            struct DbrRec* rec = elm_ctx_alloc_rec(sqlite->ctx);
            if (!rec)
                goto fail2;

            // Header.

            rec->type = DBR_ACCNT;
            rec->id = sqlite3_column_int64(stmt, ID);
            strncpy(rec->mnem,
                    (const char*)sqlite3_column_text(stmt, MNEM), DBR_MNEM_MAX);

            // Body.

            strncpy(rec->accnt.display,
                    (const char*)sqlite3_column_text(stmt, DISPLAY), DBR_DISPLAY_MAX);
            strncpy(rec->accnt.email,
                    (const char*)sqlite3_column_text(stmt, EMAIL), DBR_EMAIL_MAX);
            rec->accnt.state = NULL;

            DBR_DEBUG3F("accnt: id=%ld,mnem=%.16s,display=%.64s,email=%.64s",
                       rec->id, rec->mnem, rec->accnt.display, rec->accnt.email);

            ash_queue_push(&rq, &rec->model_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            elm_err_set(DBR_EDBSQL, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = rq.first;
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    free_recs(sqlite->ctx, &rq);
    *first = NULL;
 fail1:
    return -1;
}

static ssize_t
select_order(struct FigSqlite* sqlite, struct DbrSlNode** first)
{
    enum {
        ID,
        REV,
        STATUS,
        TRADER,
        ACCNT,
        REF,
        MARKET,
        ACTION,
        TICKS,
        RESD,
        EXEC,
        LOTS,
        MIN,
        FLAGS,
        CREATED,
        MODIFIED
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_ORDER_SQL);
    if (!stmt)
        goto fail1;

    struct AshQueue oq;
    ash_queue_init(&oq);

    ssize_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            const DbrIden id = sqlite3_column_int64(stmt, ID);
            struct DbrOrder* order = elm_ctx_alloc_order(sqlite->ctx, id);
            if (!order)
                goto fail2;

            order->id = id;
            order->level = NULL;
            order->rev = sqlite3_column_int(stmt, REV);
            order->status = sqlite3_column_int(stmt, STATUS);
            order->trader.id = sqlite3_column_int64(stmt, TRADER);
            order->accnt.id = sqlite3_column_int64(stmt, ACCNT);
            if (sqlite3_column_type(stmt, REF) != SQLITE_NULL)
                strncpy(order->ref,
                        (const char*)sqlite3_column_text(stmt, REF), DBR_REF_MAX);
            else
                order->ref[0] = '\0';
            order->market.id = sqlite3_column_int64(stmt, MARKET);
            order->action = sqlite3_column_int(stmt, ACTION);
            order->ticks = sqlite3_column_int64(stmt, TICKS);
            order->resd = sqlite3_column_int64(stmt, RESD);
            order->exec = sqlite3_column_int64(stmt, EXEC);
            order->lots = sqlite3_column_int64(stmt, LOTS);
            order->min = sqlite3_column_int64(stmt, MIN);
            order->flags = sqlite3_column_int64(stmt, FLAGS);
            order->created = sqlite3_column_int64(stmt, CREATED);
            order->modified = sqlite3_column_int64(stmt, MODIFIED);

            DBR_DEBUG3F("order: id=%ld,rev=%d,status=%d,trader=%ld,accnt=%ld,ref=%.64s,"
                        "market=%ld,action=%d,ticks=%ld,resd=%ld,exec=%ld,lots=%ld,"
                        "min=%ld,flags=%ld,created=%ld,modified=%ld",
                        order->id, order->rev, order->status, order->trader.id, order->accnt.id,
                        order->ref, order->market.id, order->action, order->ticks, order->resd,
                        order->exec, order->lots, order->min, order->flags, order->created,
                        order->modified);

            ash_queue_push(&oq, &order->model_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            elm_err_set(DBR_EDBSQL, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = oq.first;
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    free_orders(sqlite->ctx, &oq);
    *first = NULL;
 fail1:
    return -1;
}

static ssize_t
select_memb(struct FigSqlite* sqlite, struct DbrSlNode** first)
{
    enum {
        ACCNT,
        TRADER
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_MEMB_SQL);
    if (!stmt)
        goto fail1;

    struct AshQueue mq;
    ash_queue_init(&mq);

    ssize_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            const DbrIden trader = sqlite3_column_int64(stmt, TRADER);
            struct DbrMemb* memb = elm_ctx_alloc_memb(sqlite->ctx, trader);
            if (!memb)
                goto fail2;

            memb->accnt.id = sqlite3_column_int64(stmt, ACCNT);
            memb->trader.id = trader;

            DBR_DEBUG3F("memb: accnt=%ld,trader=%ld", memb->accnt.id, memb->trader.id);

            ash_queue_push(&mq, &memb->model_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            elm_err_set(DBR_EDBSQL, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = mq.first;
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    free_orders(sqlite->ctx, &mq);
    *first = NULL;
 fail1:
    return -1;
}

static ssize_t
select_trade(struct FigSqlite* sqlite, struct DbrSlNode** first)
{
    enum {
        ID,
        MATCH,
        ORDER,
        ORDER_REV,
        TRADER,
        ACCNT,
        REF,
        MARKET,
        CPTY,
        ROLE,
        ACTION,
        TICKS,
        RESD,
        EXEC,
        LOTS,
        SETTL_DATE,
        CREATED,
        MODIFIED
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_TRADE_SQL);
    if (!stmt)
        goto fail1;

    struct AshQueue tq;
    ash_queue_init(&tq);

    ssize_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            const DbrIden id = sqlite3_column_int64(stmt, ID);
            struct DbrTrade* trade = elm_ctx_alloc_trade(sqlite->ctx, id);
            if (!trade)
                goto fail2;

            trade->id = id;
            trade->match = sqlite3_column_int64(stmt, MATCH);
            trade->order = sqlite3_column_int64(stmt, ORDER);
            trade->order_rev = sqlite3_column_int(stmt, ORDER_REV);
            trade->trader.id = sqlite3_column_int64(stmt, TRADER);
            trade->accnt.id = sqlite3_column_int64(stmt, ACCNT);
            if (sqlite3_column_type(stmt, REF) != SQLITE_NULL)
                strncpy(trade->ref,
                        (const char*)sqlite3_column_text(stmt, REF), DBR_REF_MAX);
            else
                trade->ref[0] = '\0';
            trade->market.id = sqlite3_column_int64(stmt, MARKET);
            trade->cpty.id = sqlite3_column_int64(stmt, CPTY);
            trade->role = sqlite3_column_int(stmt, ROLE);
            trade->action = sqlite3_column_int(stmt, ACTION);
            trade->ticks = sqlite3_column_int64(stmt, TICKS);
            trade->resd = sqlite3_column_int64(stmt, RESD);
            trade->exec = sqlite3_column_int64(stmt, EXEC);
            trade->lots = sqlite3_column_int64(stmt, LOTS);
            trade->settl_date = sqlite3_column_int(stmt, SETTL_DATE);
            trade->created = sqlite3_column_int64(stmt, CREATED);
            trade->modified = sqlite3_column_int64(stmt, MODIFIED);

            DBR_DEBUG3F("trade: id=%ld,match=%ld,order=%ld,order_rev=%d,trader=%ld,accnt=%ld,"
                        "ref=%.64s,market=%ld,cpty=%ld,role=%d,action=%d,ticks=%ld,resd=%ld,"
                        "exec=%ld,lots=%ld,settl_date=%d,created=%ld, modified=%ld",
                        trade->id, trade->match, trade->order, trade->order_rev, trade->trader.id,
                        trade->accnt.id, trade->ref, trade->market.id, trade->cpty.id, trade->role,
                        trade->action, trade->ticks, trade->resd, trade->exec, trade->lots,
                        trade->settl_date, trade->created, trade->modified);

            ash_queue_push(&tq, &trade->model_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            elm_err_set(DBR_EDBSQL, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = tq.first;
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    free_trades(sqlite->ctx, &tq);
    *first = NULL;
 fail1:
    return -1;
}

static ssize_t
select_posn(struct FigSqlite* sqlite, struct DbrSlNode** first)
{
    enum {
        ID,
        ACCNT,
        INSTR,
        SETTL_DATE,
        ACTION,
        LICKS,
        LOTS
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_POSN_SQL);
    if (!stmt)
        goto fail1;

    struct AshQueue pq;
    ash_queue_init(&pq);

    struct DbrPosn* posn = NULL;

    ssize_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            const DbrIden id = sqlite3_column_int64(stmt, ID);
            const DbrIden accnt = sqlite3_column_int64(stmt, ACCNT);

            // Posn is null for first row.
            if (posn && posn->id == id && posn->accnt.id == accnt) {

                // Set other side.

                const int action = sqlite3_column_int(stmt, ACTION);
                if (action == DBR_BUY) {
                    posn->buy_licks = sqlite3_column_int64(stmt, LICKS);
                    posn->buy_lots = sqlite3_column_int64(stmt, LOTS);
                } else {
                    assert(action == DBR_SELL);
                    posn->sell_licks = sqlite3_column_int64(stmt, LICKS);
                    posn->sell_lots = sqlite3_column_int64(stmt, LOTS);
                }
                continue;
            }

            posn = elm_ctx_alloc_posn(sqlite->ctx, id);
            if (dbr_unlikely(!posn))
                goto fail2;

            posn->id = id;
            posn->accnt.id = accnt;
            posn->instr.id = sqlite3_column_int64(stmt, INSTR);
            posn->settl_date = sqlite3_column_int(stmt, SETTL_DATE);

            const int action = sqlite3_column_int(stmt, ACTION);
            if (action == DBR_BUY) {
                posn->buy_licks = sqlite3_column_int64(stmt, LICKS);
                posn->buy_lots = sqlite3_column_int64(stmt, LOTS);
                posn->sell_licks = 0;
                posn->sell_lots = 0;
            } else {
                assert(action == DBR_SELL);
                posn->buy_licks = 0;
                posn->buy_lots = 0;
                posn->sell_licks = sqlite3_column_int64(stmt, LICKS);
                posn->sell_lots = sqlite3_column_int64(stmt, LOTS);
            }

            ash_queue_push(&pq, &posn->model_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            elm_err_set(DBR_EDBSQL, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = pq.first;
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    free_posns(sqlite->ctx, &pq);
    *first = NULL;
 fail1:
    return -1;
}

DBR_EXTERN DbrBool
fig_sqlite_init(struct FigSqlite* sqlite, struct ElmCtx* ctx, const char* path)
{
    sqlite3* db;
    int rc = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, NULL);
    if (dbr_unlikely(rc != SQLITE_OK)) {
        elm_err_set(DBR_EDBSQL, sqlite3_errmsg(db));
        // Must close even if open failed.
        goto fail1;
    }

    // Install trace function for debugging.
#if DBR_DEBUG_LEVEL >= 2
    sqlite3_trace(db, trace_sql, NULL);
#endif // DBR_DEBUG_LEVEL >= 2

#if DBR_DEBUG_LEVEL >= 1
    rc = sqlite3_db_config(db, SQLITE_DBCONFIG_ENABLE_FKEY, 1, NULL);
    if (dbr_unlikely(rc != SQLITE_OK)) {
        elm_err_set(DBR_EDBSQL, sqlite3_errmsg(db));
        goto fail1;
    }
#endif // DBR_DEBUG_LEVEL >= 1

    if (dbr_unlikely(!exec_sql(db, "PRAGMA synchronous = OFF")
                     || !exec_sql(db, "PRAGMA journal_mode = MEMORY"))) {
        elm_err_set(DBR_EDBSQL, sqlite3_errmsg(db));
        goto fail1;
    }

    sqlite3_stmt* insert_order = prepare(db, INSERT_ORDER_SQL);
    if (!insert_order)
        goto fail1;

    sqlite3_stmt* update_order = prepare(db, UPDATE_ORDER_SQL);
    if (!update_order)
        goto fail2;

    sqlite3_stmt* archive_order = prepare(db, ARCHIVE_ORDER_SQL);
    if (!archive_order)
        goto fail3;

    sqlite3_stmt* insert_trade = prepare(db, INSERT_TRADE_SQL);
    if (!insert_trade)
        goto fail4;

    sqlite3_stmt* archive_trade = prepare(db, ARCHIVE_TRADE_SQL);
    if (!archive_trade)
        goto fail5;

    sqlite->ctx = ctx;
    sqlite->db = db;
    sqlite->insert_order = insert_order;
    sqlite->update_order = update_order;
    sqlite->archive_order = archive_order;
    sqlite->insert_trade = insert_trade;
    sqlite->archive_trade = archive_trade;
    return true;

 fail5:
    sqlite3_finalize(insert_trade);
 fail4:
    sqlite3_finalize(archive_order);
 fail3:
    sqlite3_finalize(update_order);
 fail2:
    sqlite3_finalize(insert_order);
 fail1:
    sqlite3_close(db);
    return false;
}

DBR_EXTERN void
fig_sqlite_term(struct FigSqlite* sqlite)
{
    assert(sqlite);
    sqlite3_finalize(sqlite->archive_trade);
    sqlite3_finalize(sqlite->insert_trade);
    sqlite3_finalize(sqlite->archive_order);
    sqlite3_finalize(sqlite->update_order);
    sqlite3_finalize(sqlite->insert_order);
    sqlite3_close(sqlite->db);
}

DBR_EXTERN DbrBool
fig_sqlite_begin(struct FigSqlite* sqlite)
{
    return exec_sql(sqlite->db, "BEGIN TRANSACTION");
}

DBR_EXTERN DbrBool
fig_sqlite_commit(struct FigSqlite* sqlite)
{
    return exec_sql(sqlite->db, "COMMIT TRANSACTION");
}

DBR_EXTERN DbrBool
fig_sqlite_rollback(struct FigSqlite* sqlite)
{
    return exec_sql(sqlite->db, "ROLLBACK TRANSACTION");
}

DBR_EXTERN DbrBool
fig_sqlite_insert_order(struct FigSqlite* sqlite, const struct DbrOrder* order)
{
    return bind_insert_order(sqlite, order)
        && exec_stmt(sqlite->db, sqlite->insert_order);
}

DBR_EXTERN DbrBool
fig_sqlite_update_order(struct FigSqlite* sqlite, DbrIden id, int rev, int status,
                        DbrLots resd, DbrLots exec, DbrLots lots, DbrMillis now)
{
    return bind_update_order(sqlite, id, rev, status, resd, exec, lots, now)
        && exec_stmt(sqlite->db, sqlite->update_order);
}

DBR_EXTERN DbrBool
fig_sqlite_archive_order(struct FigSqlite* sqlite, DbrIden id, DbrMillis now)
{
    return bind_archive_order(sqlite, id, now)
        && exec_stmt(sqlite->db, sqlite->archive_order);
}

DBR_EXTERN DbrBool
fig_sqlite_insert_trade(struct FigSqlite* sqlite, const struct DbrTrade* trade)
{
    return bind_insert_trade(sqlite, trade)
        && exec_stmt(sqlite->db, sqlite->insert_trade);
}

DBR_EXTERN DbrBool
fig_sqlite_archive_trade(struct FigSqlite* sqlite, DbrIden id, DbrMillis now)
{
    return bind_archive_trade(sqlite, id, now)
        && exec_stmt(sqlite->db, sqlite->archive_trade);
}

DBR_EXTERN ssize_t
fig_sqlite_select(struct FigSqlite* sqlite, int type, struct DbrSlNode** first)
{
    ssize_t ret;
    switch (type) {
    case DBR_INSTR:
        ret = select_instr(sqlite, first);
        break;
    case DBR_MARKET:
        ret = select_market(sqlite, first);
        break;
    case DBR_TRADER:
        ret = select_trader(sqlite, first);
        break;
    case DBR_ACCNT:
        ret = select_accnt(sqlite, first);
        break;
    case DBR_ORDER:
        ret = select_order(sqlite, first);
        break;
    case DBR_MEMB:
        ret = select_memb(sqlite, first);
        break;
    case DBR_TRADE:
        ret = select_trade(sqlite, first);
        break;
    case DBR_POSN:
        ret = select_posn(sqlite, first);
        break;
    default:
        assert(false);
        *first = NULL;
        ret = 0;
    }
    return ret;
}

DBR_EXTERN struct DbrSlNode*
fig_sqlite_end(struct FigSqlite* sqlite)
{
    return NULL;
}
