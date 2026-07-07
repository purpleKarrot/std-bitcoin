// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <chrono>
#include <cstdint>
#include <format>

#include <beman/any_view/any_view.hpp>
#include <beman/any_view/any_view_options.hpp>

#include <bitcoin/amount.hpp>
#include <bitcoin/any_chain_view.hpp>
#include <bitcoin/block.hpp>
#include <bitcoin/block_header.hpp>
#include <bitcoin/script.hpp>
#include <bitcoin/transaction.hpp>
#include <bitcoin/validation_flags.hpp>
#include <bitcoin/validation_status.hpp>

namespace bitcoin {

validation_status verify(block_header const& header);
validation_status verify(block_header const& header, any_chain_view chain,
                         std::chrono::sys_seconds now);

validation_status verify(bitcoin::block const& block);
validation_status verify(bitcoin::block const& block, any_chain_view chain,
                         std::chrono::sys_seconds now);
// validation_status verify(bitcoin::block const& block, chain, now, coins);

validation_status verify(bitcoin::transaction const& tx);
validation_status verify(bitcoin::transaction const& tx, any_chain_view chain);
// validation_status verify(bitcoin::transaction const& tx, chain, coins);

validation_status verify(
  script_ref script, amount value, transaction const& tx,
  std::size_t input_index, validation_flags flags,
  beman::any_view::any_view<tx_output const,
                            beman::any_view::any_view_options::random_access
                              | beman::any_view::any_view_options::sized>
    prevouts = {});

} // namespace bitcoin
