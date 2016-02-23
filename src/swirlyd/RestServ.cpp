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

#include <swirly/ash/Time.hpp>

using namespace std;

namespace swirly {
namespace mg {

RestServ::~RestServ() noexcept = default;

void RestServ::reset(string_view sv) noexcept
{
  out_.reset();

  // Remove leading slash.
  if (sv.front() == '/')
    sv.remove_prefix(1);
  uri_.reset(sv);
}

void RestServ::httpRequest(mg_connection& nc, mg::HttpMessage data)
{
  const auto now = getTimeOfDay();
  reset(data.uri());

  if (!uri_.empty()) {

    const auto tok = uri_.top();
    uri_.pop();
    const auto method = data.method();

    if (tok == "rec") {
      if (method == "GET") {
        getRec(data, now);
      } else if (method == "POST") {
        postRec(data, now);
      } else if (method == "PUT") {
        putRec(data, now);
      } else {
        // FIXME.
      }
    } else if (tok == "sess") {
      if (method == "GET") {
        getSess(data, now);
      } else if (method == "POST") {
        postSess(data, now);
      } else if (method == "PUT") {
        putSess(data, now);
      } else if (method == "DELETE") {
        deleteSess(data, now);
      } else {
        // FIXME.
      }
    } else if (tok == "view") {
      if (method == "GET") {
        getView(data, now);
      } else {
        // FIXME.
      }
    } else {
      // FIXME.
      mg_serve_http(&nc, data.get(), httpOpts_);
      return;
    }

    const auto len = static_cast<int>(out_.size());
    mg_printf(&nc, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%.*s", len, len, out_.data());

  } else {
    // FIXME.
  }
}

void RestServ::getRec(mg::HttpMessage data, Millis now)
{
  if (!uri_.empty()) {

    const auto tok = uri_.top();
    uri_.pop();

    if (tok == "asset") {
      rest_.assets(now, out_);
    } else if (tok == "contr") {
      rest_.contrs(now, out_);
    } else if (tok == "market") {
      rest_.markets(now, out_);
    } else if (tok == "trader") {
      rest_.traders(now, out_);
    } else {
      // FIXME.
    }
  } else {
    // FIXME.
  }
}

void RestServ::postRec(mg::HttpMessage data, Millis now)
{
}

void RestServ::putRec(mg::HttpMessage data, Millis now)
{
}

void RestServ::getSess(mg::HttpMessage data, Millis now)
{
}

void RestServ::postSess(mg::HttpMessage data, Millis now)
{
}

void RestServ::putSess(mg::HttpMessage data, Millis now)
{
}

void RestServ::deleteSess(mg::HttpMessage data, Millis now)
{
}

void RestServ::getView(mg::HttpMessage data, Millis now)
{
}

} // mg
} // swirly
