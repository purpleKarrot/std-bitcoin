// SPDX-License-Identifier: BSL-1.0

module;

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

export module bitcoin:script;

export namespace bitcoin {

enum class opcode : std::uint8_t
{
  op_0 = 0x00,
  op_false = op_0,
  op_pushdata1 = 0x4c,
  op_pushdata2 = 0x4d,
  op_pushdata4 = 0x4e,
  op_1negate = 0x4f,
  op_reserved = 0x50,
  op_1 = 0x51,
  op_true = op_1,
  op_2 = 0x52,
  op_3 = 0x53,
  op_4 = 0x54,
  op_5 = 0x55,
  op_6 = 0x56,
  op_7 = 0x57,
  op_8 = 0x58,
  op_9 = 0x59,
  op_10 = 0x5a,
  op_11 = 0x5b,
  op_12 = 0x5c,
  op_13 = 0x5d,
  op_14 = 0x5e,
  op_15 = 0x5f,
  op_16 = 0x60,

  op_nop = 0x61,
  op_ver = 0x62,
  op_if = 0x63,
  op_notif = 0x64,
  op_verif = 0x65,
  op_vernotif = 0x66,
  op_else = 0x67,
  op_endif = 0x68,
  op_verify = 0x69,
  op_return = 0x6a,

  op_toaltstack = 0x6b,
  op_fromaltstack = 0x6c,
  op_2drop = 0x6d,
  op_2dup = 0x6e,
  op_3dup = 0x6f,
  op_2over = 0x70,
  op_2rot = 0x71,
  op_2swap = 0x72,
  op_ifdup = 0x73,
  op_depth = 0x74,
  op_drop = 0x75,
  op_dup = 0x76,
  op_nip = 0x77,
  op_over = 0x78,
  op_pick = 0x79,
  op_roll = 0x7a,
  op_rot = 0x7b,
  op_swap = 0x7c,
  op_tuck = 0x7d,

  op_cat = 0x7e,
  op_substr = 0x7f,
  op_left = 0x80,
  op_right = 0x81,
  op_size = 0x82,

  op_invert = 0x83,
  op_and = 0x84,
  op_or = 0x85,
  op_xor = 0x86,
  op_equal = 0x87,
  op_equalverify = 0x88,
  op_reserved1 = 0x89,
  op_reserved2 = 0x8a,

  op_1add = 0x8b,
  op_1sub = 0x8c,
  op_2mul = 0x8d,
  op_2div = 0x8e,
  op_negate = 0x8f,
  op_abs = 0x90,
  op_not = 0x91,
  op_0notequal = 0x92,
  op_add = 0x93,
  op_sub = 0x94,
  op_mul = 0x95,
  op_div = 0x96,
  op_mod = 0x97,
  op_lshift = 0x98,
  op_rshift = 0x99,
  op_booland = 0x9a,
  op_boolor = 0x9b,
  op_numequal = 0x9c,
  op_numequalverify = 0x9d,
  op_numnotequal = 0x9e,
  op_lessthan = 0x9f,
  op_greaterthan = 0xa0,
  op_lessthanorequal = 0xa1,
  op_greaterthanorequal = 0xa2,
  op_min = 0xa3,
  op_max = 0xa4,
  op_within = 0xa5,

  op_ripemd160 = 0xa6,
  op_sha1 = 0xa7,
  op_sha256 = 0xa8,
  op_hash160 = 0xa9,
  op_hash256 = 0xaa,
  op_codeseparator = 0xab,
  op_checksig = 0xac,
  op_checksigverify = 0xad,
  op_checkmultisig = 0xae,
  op_checkmultisigverify = 0xaf,

  op_nop1 = 0xb0,
  op_checklocktimeverify = 0xb1,
  op_nop2 = op_checklocktimeverify,
  op_checksequenceverify = 0xb2,
  op_nop3 = op_checksequenceverify,
  op_nop4 = 0xb3,
  op_nop5 = 0xb4,
  op_nop6 = 0xb5,
  op_nop7 = 0xb6,
  op_nop8 = 0xb7,
  op_nop9 = 0xb8,
  op_nop10 = 0xb9,

  op_checksigadd = 0xba,

  op_invalidopcode = 0xff,
};

class instruction
{
public:
  constexpr instruction() noexcept = default;
  constexpr instruction(opcode code,
                        std::span<std::byte const> immediate = {}) noexcept
    : _code(code)
    , _immediate(immediate)
  {
    assert(_has_valid_immediate(code, immediate));
  }

  [[nodiscard]] constexpr auto code() const noexcept -> opcode { return _code; }
  [[nodiscard]] constexpr auto immediate() const noexcept
    -> std::span<std::byte const>
  {
    return _immediate;
  }

  [[nodiscard]] constexpr auto pushes_data() const noexcept -> bool
  {
    return static_cast<std::uint8_t>(_code)
      <= static_cast<std::uint8_t>(opcode::op_pushdata4);
  }

private:
  [[nodiscard]] static constexpr auto _has_valid_immediate(
    opcode code, std::span<std::byte const> immediate) noexcept -> bool
  {
    auto const raw = static_cast<std::uint8_t>(code);

    if (raw < static_cast<std::uint8_t>(opcode::op_pushdata1)) {
      return immediate.size() == raw;
    }

    if (code == opcode::op_pushdata1) {
      return immediate.size() <= std::numeric_limits<std::uint8_t>::max();
    }

    if (code == opcode::op_pushdata2) {
      return immediate.size() <= std::numeric_limits<std::uint16_t>::max();
    }

    if (code == opcode::op_pushdata4) {
      return immediate.size() <= std::numeric_limits<std::uint32_t>::max();
    }

    return immediate.empty();
  }

  opcode _code{};
  std::span<std::byte const> _immediate;
};

class witness_program_ref
{
public:
  constexpr witness_program_ref(std::uint8_t version,
                                std::span<std::byte const> program) noexcept
    : _version(version)
    , _program(program)
  {
    assert(version <= 16);
    assert(program.size() >= 2);
    assert(program.size() <= 40);
  }

  [[nodiscard]] constexpr auto version() const noexcept -> std::uint8_t
  {
    return _version;
  }

  [[nodiscard]] constexpr auto program() const noexcept
    -> std::span<std::byte const>
  {
    return _program;
  }

private:
  std::uint8_t _version{};
  std::span<std::byte const> _program;
};

class script;
class script_ref;

class instruction_view : public std::ranges::view_interface<instruction_view>
{
public:
  class iterator
  {
  public:
    using iterator_concept = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = bitcoin::instruction;

    iterator() noexcept = default;

    [[nodiscard]] auto operator*() const noexcept -> value_type;
    auto operator++() noexcept -> iterator&;
    auto operator++(int) noexcept -> iterator;

    friend bool operator==(iterator const& it, std::default_sentinel_t) noexcept
    {
      return it._at_end;
    }

  private:
    explicit iterator(std::span<std::byte const> bytes) noexcept;
    void _read_next() noexcept;

    std::span<std::byte const> _remaining;
    value_type _current;
    std::size_t _current_size = 0;
    bool _at_end = true;

    friend class instruction_view;
  };

  instruction_view() noexcept = default;

  [[nodiscard]] auto begin() const noexcept -> iterator;
  [[nodiscard]] auto end() const noexcept -> std::default_sentinel_t
  {
    return {};
  }

private:
  explicit instruction_view(std::span<std::byte const> bytes) noexcept;

  std::span<std::byte const> _bytes;

  friend auto instructions(script_ref value) noexcept -> instruction_view;
};

class script_ref
{
public:
  constexpr script_ref() noexcept = default;
  constexpr explicit script_ref(std::span<std::byte const> bytes) noexcept
    : _bytes(bytes)
  {
  }

  script_ref(script const& value) noexcept;
  script_ref(script&&) = delete;
  script_ref(script const&&) = delete;

  [[nodiscard]] constexpr auto empty() const noexcept -> bool
  {
    return _bytes.empty();
  }

  friend constexpr auto as_bytes(script_ref value) noexcept
    -> std::span<std::byte const>
  {
    return value._bytes;
  }

  friend constexpr bool operator==(script_ref lhs, script_ref rhs) noexcept
  {
    return std::ranges::equal(as_bytes(lhs), as_bytes(rhs));
  }

private:
  std::span<std::byte const> _bytes;
};

class script
{
public:
  constexpr script() noexcept = default;
  explicit constexpr script(std::span<std::byte const> bytes)
    : _bytes(bytes.begin(), bytes.end())
  {
  }

  explicit constexpr script(script_ref value)
    : script(as_bytes(value))
  {
  }

  [[nodiscard]] constexpr auto empty() const noexcept -> bool
  {
    return _bytes.empty();
  }

  constexpr void clear() noexcept { _bytes.clear(); }

  auto append(opcode code) -> script&;
  auto append_data(std::span<std::byte const> data) -> script&;
  auto append_number(std::int64_t value) -> script&;
  auto append(script_ref suffix) -> script&;

  friend constexpr auto as_bytes(script const& value) noexcept
    -> std::span<std::byte const>
  {
    return value._bytes;
  }

  friend bool operator==(script const&, script const&) = default;

private:
  void _append_size(std::size_t size);
  void _append_bytes(std::span<std::byte const> bytes);

  std::vector<std::byte> _bytes;
};

[[nodiscard]] auto instructions(script_ref value) noexcept -> instruction_view;

[[nodiscard]] auto is_small_integer(opcode code) noexcept -> bool;
[[nodiscard]] auto decode_small_integer(opcode code) noexcept -> int;
[[nodiscard]] auto encode_small_integer(int value) noexcept -> opcode;

[[nodiscard]] auto is_well_formed(script_ref value) noexcept -> bool;
[[nodiscard]] auto has_valid_opcodes(script_ref value) noexcept -> bool;
[[nodiscard]] auto is_push_only(script_ref value) noexcept -> bool;
[[nodiscard]] auto is_unspendable(script_ref value) noexcept -> bool;
[[nodiscard]] auto witness_program(script_ref value) noexcept
  -> std::optional<witness_program_ref>;
[[nodiscard]] auto is_pay_to_script_hash(script_ref value) noexcept -> bool;
[[nodiscard]] auto is_pay_to_witness_script_hash(script_ref value) noexcept
  -> bool;
[[nodiscard]] auto is_pay_to_taproot(script_ref value) noexcept -> bool;

} // namespace bitcoin

export template <>
struct std::formatter<bitcoin::opcode> : std::formatter<std::string_view>
{
  auto format(bitcoin::opcode code, std::format_context& ctx) const
    -> std::format_context::iterator;
};
