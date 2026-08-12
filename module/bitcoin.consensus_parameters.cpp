// SPDX-License-Identifier: BSL-1.0

module;

#include <array>
#include <cstdint>
#include <cstdlib>
#include <limits>

export module bitcoin:consensus_parameters;

import :amount;
import :hash_id;

export namespace bitcoin {

struct consensus_parameters
{
  hash256 pow_limit = hash256{std::array{
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
  }};
};

namespace mainnet {
inline constexpr auto params = consensus_parameters{};
}

namespace testnet {

inline constexpr auto params = consensus_parameters{};

} // namespace testnet

namespace signet {

inline constexpr auto params = consensus_parameters{
  .pow_limit = hash256{std::array{
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xae},
    std::byte{0x77}, std::byte{0x03}, std::byte{0x00}, std::byte{0x00},
  }},
};

} // namespace signet

namespace regtest {

inline constexpr auto params = consensus_parameters{
  .pow_limit = hash256{std::array{
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
    std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0x7f},
  }},
};

} // namespace regtest
} // namespace bitcoin
