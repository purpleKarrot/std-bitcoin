// SPDX-License-Identifier: BSL-1.0

module;

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

module bitcoin;

namespace bitcoin {
namespace {

inline constexpr auto script_size_limit = std::size_t{10'000};
inline constexpr auto script_element_size_limit = std::size_t{520};

[[nodiscard]] constexpr auto to_u8(std::byte value) noexcept -> std::uint8_t
{
  return std::to_integer<std::uint8_t>(value);
}

[[nodiscard]] constexpr auto to_u8(opcode value) noexcept -> std::uint8_t
{
  return static_cast<std::uint8_t>(value);
}

[[nodiscard]] constexpr auto byte_eq(std::byte lhs, std::uint8_t rhs) noexcept
  -> bool
{
  return to_u8(lhs) == rhs;
}

struct parsed_instruction
{
  bitcoin::instruction value;
  std::size_t size = 0;
  bool valid = false;
};

[[nodiscard]] constexpr auto read_le16(
  std::span<std::byte const, 2> bytes) noexcept -> std::uint16_t
{
  return static_cast<std::uint16_t>(to_u8(bytes[0]))
    | (static_cast<std::uint16_t>(to_u8(bytes[1])) << 8);
}

[[nodiscard]] constexpr auto read_le32(
  std::span<std::byte const, 4> bytes) noexcept -> std::uint32_t
{
  return static_cast<std::uint32_t>(to_u8(bytes[0]))
    | (static_cast<std::uint32_t>(to_u8(bytes[1])) << 8)
    | (static_cast<std::uint32_t>(to_u8(bytes[2])) << 16)
    | (static_cast<std::uint32_t>(to_u8(bytes[3])) << 24);
}

[[nodiscard]] auto parse_instruction(std::span<std::byte const> bytes) noexcept
  -> parsed_instruction
{
  if (bytes.empty()) {
    return {};
  }

  auto const raw_opcode = to_u8(bytes.front());

  if (raw_opcode < to_u8(opcode::op_pushdata1)) {
    auto const payload_size = static_cast<std::size_t>(raw_opcode);
    if (bytes.size() < 1 + payload_size) {
      return {};
    }

    return {
      .value = instruction{static_cast<opcode>(raw_opcode),
                           bytes.subspan(1, payload_size)},
      .size = 1 + payload_size,
      .valid = true,
    };
  }

  if (raw_opcode == to_u8(opcode::op_pushdata1)) {
    if (bytes.size() < 2) {
      return {};
    }

    auto const payload_size = static_cast<std::size_t>(to_u8(bytes[1]));
    if (bytes.size() < 2 + payload_size) {
      return {};
    }

    return {
      .value = instruction{static_cast<opcode>(raw_opcode),
                           bytes.subspan(2, payload_size)},
      .size = 2 + payload_size,
      .valid = true,
    };
  }

  if (raw_opcode == to_u8(opcode::op_pushdata2)) {
    if (bytes.size() < 3) {
      return {};
    }

    auto const payload_size =
      static_cast<std::size_t>(read_le16(bytes.subspan<1, 2>()));
    if (bytes.size() < 3 + payload_size) {
      return {};
    }

    return {
      .value = instruction{static_cast<opcode>(raw_opcode),
                           bytes.subspan(3, payload_size)},
      .size = 3 + payload_size,
      .valid = true,
    };
  }

  if (raw_opcode == to_u8(opcode::op_pushdata4)) {
    if (bytes.size() < 5) {
      return {};
    }

    auto const payload_size =
      static_cast<std::size_t>(read_le32(bytes.subspan<1, 4>()));
    if (bytes.size() < 5 + payload_size) {
      return {};
    }

    return {
      .value = instruction{static_cast<opcode>(raw_opcode),
                           bytes.subspan(5, payload_size)},
      .size = 5 + payload_size,
      .valid = true,
    };
  }

  return {
    .value = instruction{static_cast<opcode>(raw_opcode)},
    .size = 1,
    .valid = true,
  };
}

[[nodiscard]] auto serialize_script_number(std::int64_t value)
  -> std::vector<std::byte>
{
  if (value == 0) {
    return {};
  }

  auto result = std::vector<std::byte>{};
  auto const negative = value < 0;
  auto abs_value = negative ? (~static_cast<std::uint64_t>(value) + 1)
                            : static_cast<std::uint64_t>(value);

  while (abs_value != 0) {
    result.push_back(std::byte{static_cast<std::uint8_t>(abs_value & 0xffu)});
    abs_value >>= 8;
  }

  if ((to_u8(result.back()) & 0x80u) != 0u) {
    result.push_back(negative ? std::byte{0x80} : std::byte{0x00});
  }
  else if (negative) {
    auto const high = static_cast<std::uint8_t>(to_u8(result.back()) | 0x80u);
    result.back() = std::byte{high};
  }

  return result;
}

[[nodiscard]] constexpr auto is_defined_opcode(opcode code) noexcept -> bool
{
  return to_u8(code) <= to_u8(opcode::op_checksigadd);
}

} // namespace

auto instruction_view::iterator::operator*() const noexcept -> value_type
{
  return _current;
}

auto instruction_view::iterator::operator++() noexcept -> iterator&
{
  if (_at_end) {
    return *this;
  }

  _remaining = _remaining.subspan(_current_size);
  _read_next();
  return *this;
}

auto instruction_view::iterator::operator++(int) noexcept -> iterator
{
  auto copy = *this;
  ++*this;
  return copy;
}

instruction_view::iterator::iterator(std::span<std::byte const> bytes) noexcept
  : _remaining(bytes)
{
  _read_next();
}

void instruction_view::iterator::_read_next() noexcept
{
  if (_remaining.empty()) {
    _current = {};
    _current_size = 0;
    _at_end = true;
    return;
  }

  auto const parsed = parse_instruction(_remaining);
  if (!parsed.valid) {
    _current = {};
    _current_size = 0;
    _at_end = true;
    return;
  }

  _current = parsed.value;
  _current_size = parsed.size;
  _at_end = false;
}

instruction_view::instruction_view(std::span<std::byte const> bytes) noexcept
  : _bytes(bytes)
{
}

auto instruction_view::begin() const noexcept -> iterator
{
  return iterator{_bytes};
}

script_ref::script_ref(script const& value) noexcept
  : _bytes(as_bytes(value))
{
}

auto instructions(script_ref value) noexcept -> instruction_view
{
  return instruction_view{as_bytes(value)};
}

auto script::append(opcode code) -> script&
{
  _bytes.push_back(std::byte{to_u8(code)});
  return *this;
}

auto script::append_data(std::span<std::byte const> data) -> script&
{
  _append_size(data.size());
  _append_bytes(data);
  return *this;
}

auto script::append_number(std::int64_t value) -> script&
{
  if (value == -1) {
    return append(opcode::op_1negate);
  }

  if (value >= 0 && value <= 16) {
    return append(encode_small_integer(static_cast<int>(value)));
  }

  auto const bytes = serialize_script_number(value);
  return append_data(bytes);
}

auto script::append(script_ref suffix) -> script&
{
  _append_bytes(as_bytes(suffix));
  return *this;
}

void script::_append_size(std::size_t size)
{
  if (size < to_u8(opcode::op_pushdata1)) {
    _bytes.push_back(std::byte{static_cast<std::uint8_t>(size)});
    return;
  }

  if (size <= 0xff) {
    _bytes.push_back(std::byte{to_u8(opcode::op_pushdata1)});
    _bytes.push_back(std::byte{static_cast<std::uint8_t>(size)});
    return;
  }

  if (size <= 0xffff) {
    _bytes.push_back(std::byte{to_u8(opcode::op_pushdata2)});
    _bytes.push_back(std::byte{static_cast<std::uint8_t>(size & 0xffu)});
    _bytes.push_back(std::byte{static_cast<std::uint8_t>((size >> 8) & 0xffu)});
    return;
  }

  _bytes.push_back(std::byte{to_u8(opcode::op_pushdata4)});
  _bytes.push_back(std::byte{static_cast<std::uint8_t>(size & 0xffu)});
  _bytes.push_back(std::byte{static_cast<std::uint8_t>((size >> 8) & 0xffu)});
  _bytes.push_back(std::byte{static_cast<std::uint8_t>((size >> 16) & 0xffu)});
  _bytes.push_back(std::byte{static_cast<std::uint8_t>((size >> 24) & 0xffu)});
}

void script::_append_bytes(std::span<std::byte const> bytes)
{
  _bytes.insert(_bytes.end(), bytes.begin(), bytes.end());
}

auto is_small_integer(opcode code) noexcept -> bool
{
  auto const raw = to_u8(code);
  return (code == opcode::op_0)
    || (raw >= to_u8(opcode::op_1) && raw <= to_u8(opcode::op_16));
}

auto decode_small_integer(opcode code) noexcept -> int
{
  assert(is_small_integer(code));

  if (code == opcode::op_0) {
    return 0;
  }

  return 1 + static_cast<int>(to_u8(code) - to_u8(opcode::op_1));
}

auto encode_small_integer(int value) noexcept -> opcode
{
  assert(value >= 0);
  assert(value <= 16);

  if (value == 0) {
    return opcode::op_0;
  }

  return static_cast<opcode>(to_u8(opcode::op_1) + value - 1);
}

auto is_well_formed(script_ref value) noexcept -> bool
{
  auto remaining = as_bytes(value);

  while (!remaining.empty()) {
    auto const parsed = parse_instruction(remaining);
    if (!parsed.valid) {
      return false;
    }

    remaining = remaining.subspan(parsed.size);
  }

  return true;
}

auto has_valid_opcodes(script_ref value) noexcept -> bool
{
  auto remaining = as_bytes(value);

  while (!remaining.empty()) {
    auto const parsed = parse_instruction(remaining);
    if (!parsed.valid
        || !is_defined_opcode(parsed.value.code())
        || (parsed.value.immediate().size() > script_element_size_limit)) {
      return false;
    }

    remaining = remaining.subspan(parsed.size);
  }

  return true;
}

auto is_push_only(script_ref value) noexcept -> bool
{
  auto remaining = as_bytes(value);

  while (!remaining.empty()) {
    auto const parsed = parse_instruction(remaining);
    if (!parsed.valid) {
      return false;
    }

    if (to_u8(parsed.value.code()) > to_u8(opcode::op_16)) {
      return false;
    }

    remaining = remaining.subspan(parsed.size);
  }

  return true;
}

auto is_unspendable(script_ref value) noexcept -> bool
{
  auto const bytes = as_bytes(value);
  return (!bytes.empty() && byte_eq(bytes.front(), to_u8(opcode::op_return)))
    || (bytes.size() > script_size_limit);
}

auto witness_program(script_ref value) noexcept
  -> std::optional<witness_program_ref>
{
  auto const bytes = as_bytes(value);

  if (bytes.size() < 4 || bytes.size() > 42) {
    return std::nullopt;
  }

  auto const version_code = static_cast<opcode>(to_u8(bytes[0]));
  if (!is_small_integer(version_code)) {
    return std::nullopt;
  }

  auto const version = decode_small_integer(version_code);

  auto const program_size = static_cast<std::size_t>(to_u8(bytes[1]));
  if (program_size < 2 || program_size > 40) {
    return std::nullopt;
  }

  if (bytes.size() != program_size + 2) {
    return std::nullopt;
  }

  return witness_program_ref{static_cast<std::uint8_t>(version),
                             bytes.subspan(2, program_size)};
}

auto is_pay_to_script_hash(script_ref value) noexcept -> bool
{
  auto const bytes = as_bytes(value);
  return (bytes.size() == 23)
    && byte_eq(bytes[0], to_u8(opcode::op_hash160))
    && byte_eq(bytes[1], 0x14)
    && byte_eq(bytes[22], to_u8(opcode::op_equal));
}

auto is_pay_to_witness_script_hash(script_ref value) noexcept -> bool
{
  auto const bytes = as_bytes(value);
  return (bytes.size() == 34)
    && byte_eq(bytes[0], to_u8(opcode::op_0))
    && byte_eq(bytes[1], 0x20);
}

auto is_pay_to_taproot(script_ref value) noexcept -> bool
{
  auto const bytes = as_bytes(value);
  return (bytes.size() == 34)
    && byte_eq(bytes[0], to_u8(opcode::op_1))
    && byte_eq(bytes[1], 0x20);
}

} // namespace bitcoin
