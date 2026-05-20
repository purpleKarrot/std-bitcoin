// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/chain.hpp>

#include <algorithm>
#include <array>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include <doctest/doctest.h>

namespace {

[[nodiscard]] auto make_block_header(std::byte marker) -> bitcoin::block_header
{
  auto raw = std::array<std::byte, 80>{};
  raw[0] = marker;
  raw[76] = marker;

  auto const header = bitcoin::parse_block_header(raw);
  REQUIRE(header.has_value());
  return *header;
}

class pointer_chain_view : public std::ranges::view_base
{
public:
  pointer_chain_view() = default;

  pointer_chain_view(bitcoin::block_header const* data, std::size_t count)
    : _data(data)
    , _count(count)
  {
  }

  [[nodiscard]] auto begin() const { return _data; }
  [[nodiscard]] auto end() const { return _data + _count; }
  [[nodiscard]] auto size() const { return _count; }

  [[nodiscard]] auto mismatch(pointer_chain_view const& other) const
    -> std::ranges::mismatch_result<bitcoin::block_header const*,
                                    bitcoin::block_header const*>
  {
    return std::ranges::mismatch(*this, other);
  }

  [[nodiscard]] auto starts_with(pointer_chain_view const& other) const -> bool
  {
    return std::ranges::equal(other, *this | std::views::take(other.size()));
  }

private:
  bitcoin::block_header const* _data = nullptr;
  std::size_t _count = 0;
};

} // namespace

static_assert(bitcoin::chain<std::span<bitcoin::block_header>>);
static_assert(bitcoin::chain<pointer_chain_view>);
static_assert(!bitcoin::chain<std::vector<bitcoin::block_header>>);
static_assert(
  std::same_as<decltype(std::declval<bitcoin::any_chain const&>().mismatch(
                 std::declval<bitcoin::any_chain const&>())),
               std::ranges::mismatch_result<bitcoin::any_chain::iterator,
                                            bitcoin::any_chain::iterator>>);

TEST_CASE("any_chain default construction")
{
  CHECK(bitcoin::any_chain{}.empty());
}

TEST_CASE("any_chain constructs from lvalue chain view without operator[]")
{
  auto headers = std::array{
    make_block_header(std::byte{1}),
    make_block_header(std::byte{2}),
  };
  auto view = pointer_chain_view(headers.data(), headers.size());

  bitcoin::any_chain chain = view;

  CHECK(chain.size() == headers.size());
  CHECK(chain[0] == headers[0]);
  CHECK(chain[1] == headers[1]);
}

TEST_CASE("any_chain mismatch returns ranges mismatch_result")
{
  auto lhs_headers = std::array{
    make_block_header(std::byte{1}),
    make_block_header(std::byte{2}),
  };
  auto rhs_headers = std::array{
    make_block_header(std::byte{1}),
  };

  bitcoin::any_chain lhs =
    pointer_chain_view(lhs_headers.data(), lhs_headers.size());
  bitcoin::any_chain rhs =
    pointer_chain_view(rhs_headers.data(), rhs_headers.size());

  auto const result = lhs.mismatch(rhs);

  CHECK(result.in1 == lhs.begin() + 1);
  CHECK(result.in2 == rhs.end());
  CHECK(lhs.starts_with(rhs));
}

TEST_CASE("any_chain starts_with handles exact prefix and divergence")
{
  auto full_headers = std::array{
    make_block_header(std::byte{1}),
    make_block_header(std::byte{2}),
    make_block_header(std::byte{3}),
  };
  auto prefix_headers = std::array{
    make_block_header(std::byte{1}),
    make_block_header(std::byte{2}),
  };
  auto divergent_headers = std::array{
    make_block_header(std::byte{1}),
    make_block_header(std::byte{4}),
  };

  bitcoin::any_chain full =
    pointer_chain_view(full_headers.data(), full_headers.size());
  bitcoin::any_chain exact =
    pointer_chain_view(full_headers.data(), full_headers.size());
  bitcoin::any_chain prefix =
    pointer_chain_view(prefix_headers.data(), prefix_headers.size());
  bitcoin::any_chain divergent =
    pointer_chain_view(divergent_headers.data(), divergent_headers.size());
  bitcoin::any_chain empty = pointer_chain_view();

  CHECK(full.starts_with(exact));
  CHECK(full.starts_with(prefix));
  CHECK_FALSE(full.starts_with(divergent));
  CHECK_FALSE(prefix.starts_with(full));
  CHECK(full.starts_with(empty));
}
