// SPDX-License-Identifier: BSL-1.0

import bitcoin;
import std;

#include <doctest/doctest.h>

namespace {

class pointer_chain : public std::ranges::view_base
{
public:
  pointer_chain() = default;
  pointer_chain(bitcoin::block_header const* data, std::size_t count)
    : _data{data}
    , _count{count}
  {
  }

  [[nodiscard]] auto begin() const { return _data; }
  [[nodiscard]] auto end() const { return _data + _count; }
  [[nodiscard]] auto size() const { return _count; }

private:
  bitcoin::block_header const* _data = nullptr;
  std::size_t _count = 0;
};

class noncopyable_chain
{
public:
  noncopyable_chain() = default;
  noncopyable_chain(noncopyable_chain const&) = delete;
  noncopyable_chain& operator=(noncopyable_chain const&) = delete;

  [[nodiscard]] auto begin() const { return _headers.begin(); }
  [[nodiscard]] auto end() const { return _headers.end(); }
  [[nodiscard]] auto size() const { return _headers.size(); }

private:
  std::array<bitcoin::block_header, 1> _headers{};
};

class move_only_chain : public std::ranges::view_base
{
public:
  move_only_chain() = default;
  move_only_chain(move_only_chain const&) = delete;
  move_only_chain& operator=(move_only_chain const&) = delete;
  move_only_chain(move_only_chain&&) = default;
  move_only_chain& operator=(move_only_chain&&) = default;

  [[nodiscard]] auto begin() const { return _headers.begin(); }
  [[nodiscard]] auto end() const { return _headers.end(); }
  [[nodiscard]] auto size() const { return _headers.size(); }

private:
  std::span<bitcoin::block_header const> _headers;
};

class mutable_only_chain
{
public:
  [[nodiscard]] auto begin() { return _headers.begin(); }
  [[nodiscard]] auto end() { return _headers.end(); }
  [[nodiscard]] auto size() const { return _headers.size(); }

private:
  std::array<bitcoin::block_header, 1> _headers{};
};

} // namespace

static_assert(bitcoin::chain<std::span<bitcoin::block_header const>>);
static_assert(bitcoin::chain<pointer_chain>);
static_assert(bitcoin::chain<std::vector<bitcoin::block_header>>);
static_assert(bitcoin::chain<std::vector<bitcoin::block_header> const&>);
static_assert(bitcoin::chain<noncopyable_chain const&>);
static_assert(bitcoin::chain<move_only_chain>);
static_assert(bitcoin::chain<mutable_only_chain&>);
static_assert(!bitcoin::chain<mutable_only_chain const&>);
static_assert(!bitcoin::chain<std::list<bitcoin::block_header>>);
static_assert(!bitcoin::chain<std::vector<int>>);
static_assert(
  !std::invocable<bitcoin::verifier const&, bitcoin::block_header const&,
                  mutable_only_chain&, std::chrono::sys_seconds>);

TEST_CASE("header validation borrows chain ranges")
{
  auto header = bitcoin::block_header{};
  auto now = std::chrono::sys_seconds{};
  auto headers = std::vector{header};
  auto const const_headers = headers;

  auto rejects_parent = [&](auto const& chain) {
    auto status = bitcoin::verify(header, chain, now);
    CHECK(std::format("{}", status) == "parent not found");
  };

  rejects_parent(headers);
  rejects_parent(const_headers);
  rejects_parent(std::vector{header});
  rejects_parent(std::span{headers});
  rejects_parent(noncopyable_chain{});
  rejects_parent(move_only_chain{});

  CHECK(headers == const_headers);
}
