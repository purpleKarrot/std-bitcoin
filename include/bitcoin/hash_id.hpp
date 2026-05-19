// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <bitcoin/detail/basic_hash_id.hpp> // IWYU pragma: export

namespace bitcoin::detail {

struct hash256_tag;
struct txid_tag;
struct wtxid_tag;
struct block_hash_tag;

} // namespace bitcoin::detail

namespace bitcoin {

using hash256 = detail::basic_hash_id<detail::hash256_tag>;
using txid = detail::basic_hash_id<detail::txid_tag>;
using wtxid = detail::basic_hash_id<detail::wtxid_tag>;
using block_hash = detail::basic_hash_id<detail::block_hash_tag>;

} // namespace bitcoin
