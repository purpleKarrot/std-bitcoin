// SPDX-License-Identifier: BSL-1.0

module;

#include <ranges>

#include "validation.h"

module bitcoin;
import legacy;

namespace bitcoin {

validation_status verifier::verify(bitcoin::block const& b) const
{
  if (auto status = verify(b.header()); !status.ok()) {
    return status;
  }

  auto state = BlockValidationState{};
  auto result = CheckBlock(legacy::convert_block(b), state,
                           legacy::convert_consensus(*_params));
  return result ? 0 : -1; // state.GetResult();
}

validation_status verifier::verify(bitcoin::block const& b,
                                   any_chain_view chain, // NOLINT
                                   std::chrono::sys_seconds now) const
{
  if (auto status = verify(b.header(), chain, now); !status.ok()) {
    return status;
  }

  auto state = BlockValidationState{};
  auto result = ContextualCheckBlock(
    legacy::convert_block(b), state, legacy::convert_consensus(*_params),
    AnyChainView{chain | std::views::transform(legacy::convert_header)});
  return result ? 0 : -1; // state.GetResult();
}

validation_status verifier::verify(bitcoin::block const& block,
                                   any_chain_view chain,
                                   std::chrono::sys_seconds now,
                                   coin_index_ref coins) const
{
  class coin_map
  {
  public:
    using mapped_type = coin_impl;

    coin_map(coin_index_ref global)
      : _global{global}
    {
    }

    void emplace(bitcoin::outpoint const& p, coin_impl const& c)
    {
      _local.emplace(p, c);
    }

    void erase(bitcoin::outpoint const& p) { _local.emplace(p, std::nullopt); }

    std::optional<coin_impl> lookup(bitcoin::outpoint const& p) const
    {
      if (auto it = _local.find(p); it != _local.end()) {
        return it->second;
      }
      return _global.lookup(p);
    }

  private:
    std::unordered_map<bitcoin::outpoint, std::optional<coin_impl>> _local;
    coin_index_ref _global;
  };

  static_assert(coin_index<coin_map>);

  return -1;
}

} // namespace bitcoin
