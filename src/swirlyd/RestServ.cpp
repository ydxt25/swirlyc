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
#include "RestServ.hpp"

#include <swirly/fir/Rest.hpp>

#include <swirly/elm/Exception.hpp>

#include <swirly/ash/Finally.hpp>
#include <swirly/ash/Log.hpp>
#include <swirly/ash/Time.hpp>

#include <chrono>

using namespace std;

namespace swirly {
namespace mg {

RestServ::~RestServ() noexcept = default;

void RestServ::reset(mg::HttpMessage data) noexcept
{
  state_ = 0;

  const auto method = data.method();
  if (method == "GET") {
    state_ |= MethodGet;
  } else if (method == "POST") {
    state_ |= MethodPost;
  } else if (method == "PUT") {
    state_ |= MethodPut;
  } else if (method == "DELETE") {
    state_ |= MethodDelete;
  }

  auto uri = data.uri();
  // Remove leading slash.
  if (uri.front() == '/') {
    uri.remove_prefix(1);
  }
  uri_.reset(uri);
}

void RestServ::httpRequest(mg_connection& nc, mg::HttpMessage data)
{
  using namespace chrono;

  const auto start = high_resolution_clock::now();
  auto finally = makeFinally([&data, &start]() {
    const auto end = high_resolution_clock::now();
    const auto usec = duration_cast<microseconds>(end - start);
    SWIRLY_INFO(logMsg() << data.method() << ' ' << data.uri() << " processed in " << usec.count()
                         << " usec");
  });

  reset(data);

  if (uri_.empty() || uri_.top() != "api") {
    mg_serve_http(&nc, data.get(), httpOpts_);
    return;
  }
  uri_.pop();

  const auto now = getTimeOfDay();
  // See mg_send().
  nc.last_io_time = unbox(now) / 1000;

  StreamBuf buf{nc.send_mbuf};
  out_.rdbuf(&buf);
  out_.reset(200, "OK");
  try {
    restRequest(data, now);
    if (!isSet(MatchUri)) {
      throw NotFoundException{errMsg() << "resource '" << data.uri() << "' does not exist"};
    }
    if (!isSet(MatchMethod)) {
      throw MethodNotAllowedException{errMsg() << "method '" << data.method()
                                               << "' is not allowed"};
    }
  } catch (const ServException& e) {
    out_.reset(e.httpStatus(), e.httpReason());
    out_ << e;
  } catch (const exception& e) {
    const int status{500};
    const char* const reason{"Internal Server Error"};
    out_.reset(status, reason);
    ServException::toJson(status, reason, e.what(), out_);
  }
  out_.setContentLength();
}

void RestServ::restRequest(mg::HttpMessage data, Millis now)
{
  if (uri_.empty()) {
    // /api
    return;
  }

  const auto tok = uri_.top();
  uri_.pop();

  if (tok == "rec") {
    // /api/rec
    recRequest(data, now);
  } else if (tok == "sess") {
    // /api/sess
    sessRequest(data, now);
  } else if (tok == "view") {
    // /api/view
    viewRequest(data, now);
  }
}

void RestServ::recRequest(mg::HttpMessage data, Millis now)
{
  if (uri_.empty()) {

    // /api/rec
    state_ |= MatchUri;

    if (isSet(MethodGet)) {
      // GET /api/rec
      state_ |= MatchMethod;
      const int bs{EntitySet::Asset | EntitySet::Contr | EntitySet::Market};
      rest_.getRec(bs, now, out_);
    }
    return;
  }

  const auto tok = uri_.top();
  uri_.pop();

  const auto es = EntitySet::parse(tok);
  if (es.many()) {

    if (uri_.empty()) {

      // /api/rec/entity,entity...
      state_ |= MatchUri;

      if (isSet(MethodGet)) {
        // GET /api/rec/entity,entity...
        state_ |= MatchMethod;
        rest_.getRec(es, now, out_);
      }
    }
    return;
  }

  switch (es.get()) {
  case EntitySet::Asset:
    assetRequest(data, now);
    break;
  case EntitySet::Contr:
    contrRequest(data, now);
    break;
  case EntitySet::Market:
    marketRequest(data, now);
    break;
  case EntitySet::Trader:
    traderRequest(data, now);
    break;
  }
}

void RestServ::assetRequest(mg::HttpMessage data, Millis now)
{
  if (uri_.empty()) {

    // /api/rec/asset
    state_ |= MatchUri;

    if (isSet(MethodGet)) {
      // GET /api/rec/asset
      state_ |= MatchMethod;
      rest_.getAsset(now, out_);
    }
    return;
  }

  const auto mnem = uri_.top();
  uri_.pop();

  if (uri_.empty()) {

    // /api/rec/asset/MNEM
    state_ |= MatchUri;

    if (isSet(MethodGet)) {
      // GET /api/rec/asset/MNEM
      state_ |= MatchMethod;
      rest_.getAsset(mnem, now, out_);
    }
    return;
  }
}

void RestServ::contrRequest(mg::HttpMessage data, Millis now)
{
  if (uri_.empty()) {

    // /api/rec/contr
    state_ |= MatchUri;

    if (isSet(MethodGet)) {
      // GET /api/rec/contr
      state_ |= MatchMethod;
      rest_.getContr(now, out_);
    }
    return;
  }

  const auto mnem = uri_.top();
  uri_.pop();

  if (uri_.empty()) {

    // /api/rec/contr/MNEM
    state_ |= MatchUri;

    if (isSet(MethodGet)) {
      // GET /api/rec/contr/MNEM
      state_ |= MatchMethod;
      rest_.getContr(mnem, now, out_);
    }
    return;
  }
}

void RestServ::marketRequest(mg::HttpMessage data, Millis now)
{
  if (uri_.empty()) {

    // /api/rec/market
    state_ |= MatchUri;

    switch (state_ & MethodMask) {
    case MethodGet:
      // GET /api/rec/market
      state_ |= MatchMethod;
      rest_.getMarket(now, out_);
      break;
    case MethodPost:
      // POST /api/rec/market
      state_ |= MatchMethod;
      rest_.postMarket(now, out_);
      break;
    }
    return;
  }

  const auto mnem = uri_.top();
  uri_.pop();

  if (uri_.empty()) {

    // /api/rec/market/MNEM
    state_ |= MatchUri;

    switch (state_ & MethodMask) {
    case MethodGet:
      // GET /api/rec/market/MNEM
      state_ |= MatchMethod;
      rest_.getMarket(mnem, now, out_);
      break;
    case MethodPut:
      // PUT /api/rec/market/MNEM
      state_ |= MatchMethod;
      rest_.putMarket(mnem, now, out_);
      break;
    }
    return;
  }
}

void RestServ::traderRequest(mg::HttpMessage data, Millis now)
{
  if (uri_.empty()) {

    // /api/rec/trader
    state_ |= MatchUri;

    switch (state_ & MethodMask) {
    case MethodGet:
      // GET /api/rec/trader
      state_ |= MatchMethod;
      rest_.getTrader(now, out_);
      break;
    case MethodPost:
      // POST /api/rec/market
      state_ |= MatchMethod;
      rest_.postTrader(now, out_);
      break;
    }
    return;
  }

  const auto mnem = uri_.top();
  uri_.pop();

  if (uri_.empty()) {

    // /api/rec/trader/MNEM
    state_ |= MatchUri;

    switch (state_ & MethodMask) {
    case MethodGet:
      // GET /api/rec/trader/MNEM
      state_ |= MatchMethod;
      rest_.getTrader(mnem, now, out_);
      break;
    case MethodPut:
      // PUT /api/rec/trader/MNEM
      state_ |= MatchMethod;
      rest_.putTrader(mnem, now, out_);
      break;
    }
    return;
  }
}

void RestServ::sessRequest(mg::HttpMessage data, Millis now)
{
  if (uri_.empty()) {

    // /api/sess
    state_ |= MatchUri;

    if (isSet(MethodGet)) {
      // GET /api/sess
      state_ |= MatchMethod;
      const int bs{EntitySet::Order | EntitySet::Trade | EntitySet::Posn | EntitySet::View};
      rest_.getSess(bs, now, out_);
    }
    return;
  }

  const auto tok = uri_.top();
  uri_.pop();

  const auto es = EntitySet::parse(tok);
  if (es.many()) {

    if (uri_.empty()) {

      // /api/sess/entity,entity...
      state_ |= MatchUri;

      if (isSet(MethodGet)) {
        // GET /api/sess/entity,entity...
        state_ |= MatchMethod;
        rest_.getSess(es, now, out_);
      }
    }
    return;
  }

  switch (es.get()) {
  case EntitySet::Order:
    orderRequest(data, now);
    break;
  case EntitySet::Trade:
    tradeRequest(data, now);
    break;
  case EntitySet::Posn:
    posnRequest(data, now);
    break;
  case EntitySet::View:
    viewRequest(data, now);
    break;
  }
}

void RestServ::orderRequest(mg::HttpMessage data, Millis now)
{
  if (uri_.empty()) {

    // /api/sess/order
    state_ |= MatchUri;

    if (isSet(MethodGet)) {
      // GET /api/sess/order
      state_ |= MatchMethod;
      rest_.getOrder(now, out_);
    }
    return;
  }

  const auto market = uri_.top();
  uri_.pop();

  if (uri_.empty()) {

    // /api/sess/order/MARKET
    state_ |= MatchUri;

    switch (state_ & MethodMask) {
    case MethodGet:
      // GET /api/sess/order/MARKET
      state_ |= MatchMethod;
      rest_.getOrder(market, now, out_);
      break;
    case MethodPost:
      // POST /api/sess/order/MARKET
      state_ |= MatchMethod;
      rest_.postOrder(market, now, out_);
      break;
    }
    return;
  }

  parseIds(uri_.top());
  uri_.pop();

  if (uri_.empty()) {

    // /api/sess/order/MARKET/ID,ID...
    state_ |= MatchUri;

    switch (state_ & MethodMask) {
    case MethodGet:
      // GET /api/sess/order/MARKET/ID
      state_ |= MatchMethod;
      rest_.getOrder(market, ids_[0], now, out_);
      break;
    case MethodPut:
      // PUT /api/sess/order/MARKET/ID,ID...
      state_ |= MatchMethod;
      rest_.putOrder(market, ids(), now, out_);
      break;
    case MethodDelete:
      // DELETE /api/sess/order/MARKET/ID,ID...
      state_ |= MatchMethod;
      rest_.deleteOrder(market, ids(), now, out_);
      break;
    }
    return;
  }
}

void RestServ::tradeRequest(mg::HttpMessage data, Millis now)
{
  if (uri_.empty()) {

    // /api/sess/trade
    state_ |= MatchUri;

    if (isSet(MethodGet)) {
      // GET /api/sess/trade
      state_ |= MatchMethod;
      rest_.getTrade(now, out_);
    }
    return;
  }

  const auto market = uri_.top();
  uri_.pop();

  if (uri_.empty()) {

    // /api/sess/trade/MARKET
    state_ |= MatchUri;

    switch (state_ & MethodMask) {
    case MethodGet:
      // GET /api/sess/trade/MARKET
      state_ |= MatchMethod;
      rest_.getTrade(market, now, out_);
      break;
    case MethodPost:
      // POST /api/sess/trade/MARKET
      state_ |= MatchMethod;
      rest_.postTrade(market, now, out_);
      break;
    }
    return;
  }

  parseIds(uri_.top());
  uri_.pop();

  if (uri_.empty()) {

    // /api/sess/trade/MARKET/ID,ID...
    state_ |= MatchUri;

    switch (state_ & MethodMask) {
    case MethodGet:
      // GET /api/sess/trade/MARKET/ID
      state_ |= MatchMethod;
      rest_.getTrade(market, ids_[0], now, out_);
      break;
    case MethodDelete:
      // DELETE /api/sess/trade/MARKET/ID,ID...
      state_ |= MatchMethod;
      rest_.deleteTrade(market, ids(), now, out_);
      break;
    }
    return;
  }
}

void RestServ::posnRequest(mg::HttpMessage data, Millis now)
{
  if (uri_.empty()) {

    // /api/sess/posn
    state_ |= MatchUri;

    if (isSet(MethodGet)) {
      // GET /api/sess/posn
      state_ |= MatchMethod;
      rest_.getPosn(now, out_);
    }
    return;
  }

  const auto contr = uri_.top();
  uri_.pop();

  if (uri_.empty()) {

    // /api/sess/posn/CONTR
    state_ |= MatchUri;

    if (isSet(MethodGet)) {
      // GET /api/sess/posn/CONTR
      state_ |= MatchMethod;
      rest_.getPosn(contr, now, out_);
    }
    return;
  }

  const auto settlDate = box<IsoDate>(stoul(uri_.top()));
  uri_.pop();

  if (uri_.empty()) {

    // /api/sess/posn/CONTR/SETTL_DATE
    state_ |= MatchUri;

    if (isSet(MethodGet)) {
      // GET /api/sess/posn/CONTR/SETTL_DATE
      state_ |= MatchMethod;
      rest_.getPosn(contr, settlDate, now, out_);
    }
    return;
  }
}

void RestServ::viewRequest(mg::HttpMessage data, Millis now)
{
  if (uri_.empty()) {

    // /api/view
    state_ |= MatchUri;

    if (isSet(MethodGet)) {
      // GET /api/view
      state_ |= MatchMethod;
      rest_.getView(now, out_);
    }
    return;
  }

  const auto market = uri_.top();
  uri_.pop();

  if (uri_.empty()) {

    // /api/view/MARKET
    state_ |= MatchUri;

    if (isSet(MethodGet)) {
      // GET /api/view/MARKET
      state_ |= MatchMethod;
      rest_.getView(market, now, out_);
    }
    return;
  }
}

void RestServ::parseIds(string_view sv) noexcept
{
  ids_.clear();
  Tokeniser<','> toks{sv};
  while (!toks.empty()) {
    ids_.push_back(static_cast<Iden>(stoul(toks.top())));
    toks.pop();
  }
}

} // mg
} // swirly