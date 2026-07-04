// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <bitcoin/detail/basic_hash_id.hpp> // IWYU pragma: export

namespace bitcoin {

class block;
struct block_header;
class transaction;

namespace detail {

struct block_hash_policy
{
  void operator()(block const& b, std::span<std::byte, 32> dst) const;
  void operator()(block_header const& hdr, std::span<std::byte, 32> dst) const;
};

struct txid_policy
{
  void operator()(transaction const& tx, std::span<std::byte, 32> dst) const;
};

struct wtxid_policy
{
  void operator()(transaction const& tx, std::span<std::byte, 32> dst) const;
};

struct hash256_policy
{
};

} // namespace detail

using block_hash = detail::basic_hash_id<detail::block_hash_policy>;
using txid = detail::basic_hash_id<detail::txid_policy>;
using wtxid = detail::basic_hash_id<detail::wtxid_policy>;
using hash256 = detail::basic_hash_id<detail::hash256_policy>;

} // namespace bitcoin
