# Adding Bitcoin Vocabulary Types to the C++ Standard Library

**Document number:** DXXXXR0\
**Date:** 2025-07-14\
**Audience:** LEWG, SG14 (Low-Latency / Financial)\
**Reply-to:** TBD

______________________________________________________________________

> **Note — For illustrative purposes only.**\
> This document is written in the style of a WG21 standardization paper. It has
> not been submitted to the ISO C++ committee and is not under active
> consideration for standardization.

______________________________________________________________________

## Table of Contents

1. [Abstract](#abstract)
2. [Motivation and Scope](#motivation-and-scope)
3. [Impact on the Standard](#impact-on-the-standard)
4. [Design Decisions](#design-decisions)
5. [Technical Specifications](#technical-specifications)
6. [References](#references)

______________________________________________________________________

## Abstract

This paper proposes adding a set of vocabulary types for the Bitcoin protocol to
the C++ Standard Library under the header `<bitcoin>`. These types live in
`namespace bitcoin`. The goal is stable, zero-overhead, strongly-typed building
blocks that interoperate across libraries without each defining their own
incompatible representations.

______________________________________________________________________

## Motivation and Scope

Bitcoin and the broader ecosystem of protocols built on top of it (Lightning
Network, Liquid, Ark, …) are implemented in dozens of independent C++ libraries.
Every one of them defines its own `uint256`, `CTransaction`, `CTxOut`, or
equivalent. These types are structurally identical yet source-incompatible,
forcing needless conversion layers, copies, and impedance-mismatch bugs at every
library boundary.

The situation mirrors the pre-`std::chrono` world, where every project had its
own time duration type. `<chrono>` solved that class of problem by giving
duration a permanent, well-specified home in the standard. We propose the same
treatment for Bitcoin's core wire-protocol types.

### In scope

- 32-byte hash types: `hash256`, `txid`, `wtxid`, `block_hash`
- Monetary value: `amount`
- Script byte container: `script`
- Transaction structure: `outpoint`, `tx_input`, `tx_output`, `transaction`
- Block structure: `block_header`, `block`

### Out of scope

- Script execution / interpreter
- Cryptographic primitives (hashing, ECDSA, Schnorr)
- Networking / P2P protocol
- Wallet key derivation (BIP-32, BIP-39)
- Consensus validation logic

______________________________________________________________________

## Impact on the Standard

This is a pure library addition. It requires no core language changes. It
depends on:

- `<array>`, `<concepts>`, `<cstddef>`, `<cstdint>`
- `<compare>`, `<format>`, `<ranges>`, `<span>`, `<stdexcept>`

No existing names are modified or deprecated.

______________________________________________________________________

## Design Decisions

### D1 — Strong types over raw integers and byte arrays

`txid`, `wtxid`, `block_hash`, and `hash256` are all 32-byte digest values, but
they must not be implicitly interconvertible. A function accepting a `txid` must
not silently accept a `block_hash` or a `wtxid`. This rules out aliasing them to
the same underlying type (for example, `using txid = std::array<std::byte, 32>`).
The proposal models these as distinct specializations of an exposition-only
template `basic-hash-id<Tag>`, producing separate, non-interconvertible types
with identical storage representation.

### D2 — Wire byte order internally; display byte order externally

Bitcoin's double-SHA256 digests are stored on the wire in little-endian (natural
hash) byte order. Block explorers, wallet software, and developer tooling
display them byte-reversed. The types store wire order internally. `std::format`
/ `std::to_string` produce the byte-reversed hex representation that every
existing tool expects.

### D3 — `amount` is not `int64_t`

Satoshi amounts need overflow-checked arithmetic and must never be confused with
unrelated integers. `bitcoin::amount` wraps `int64_t`, exposes named constants
(`coin()`, `max_money()`), and provides `+`, `-`, `*` (scalar), and `/` (scalar)
only through checked operations.

### D4 — `script` is an opaque byte container at vocabulary level

Pattern-matching for `P2PKH`, `P2TR`, opcode enumeration, and script execution
belong to a higher-level facility (not proposed here). At vocabulary level
`script` is an opaque container exposing only a `std::span<const std::byte>`
view.

### D5 — Witness data belongs to each input

Each `tx_input` exposes its witness data through a `witness()` observer
returning an implementation-defined range of byte strings. An empty range
indicates a non-SegWit input. The BIP-141 wire-format segregation of witness
data to the end of the transaction is an encoding artifact and imposes no
constraint on the vocabulary-level design. Implementations are free to store
witness data co-located with each input or in a separate parallel structure.

### D6 — Opaque classes; no exposed storage types

All types are `class` with private data members and accessor-only public
interfaces. Collection accessors return an unspecified type satisfying the
*value-range*<T> named requirement (see D10) rather than `const std::vector<T>&`,
giving implementations full freedom over their backing store.

### D7 — User-defined literals live in `bitcoin::literals`

Opt-in literals (`_sat`, `_btc`) are placed in the sub-namespace
`bitcoin::literals`, following the same convention as `std::chrono_literals` and
`std::string_literals`. Callers bring them into scope with
`using namespace bitcoin::literals`.

### D8 — Script accessor naming drops Hungarian Notation

Bitcoin Core names the input authorization script `scriptSig` and the output
locking script `scriptPubKey`. The `script` prefix is type information — a habit
carried over from Satoshi's use of Hungarian Notation — that adds no meaning in
a typed API. The more descriptive alternatives (`input_script`, `output_script`)
are redundant given that the owning class already provides that context. Both
accessors are therefore simply named `script()`, on `tx_input` and `tx_output`
respectively.

### D9 — ABI stability strategy is implementation-defined

This paper does not mandate any particular ABI versioning scheme. Whether to
use per-type `inline namespace` versioning (e.g. `inline namespace
transaction_v1`), a single top-level `inline namespace bitcoin_v1`, or no
versioning at all is an implementer's choice. The paper requires only that all
types and constants are accessible as `bitcoin::hash256`, `bitcoin::amount`,
etc. — i.e. as direct members of `namespace bitcoin`.

### D10 — Exposition-only range helpers

One exposition-only helper constrains return types of collection accessors in
this paper. It is not part of the public API.

*value-range*<T> is a **named requirement** for the unspecified return types of
collection accessors. A type `R` satisfies *value-range*<T> if it models
`std::ranges::view`, `std::ranges::sized_range`, and
`std::ranges::random_access_range`, and `std::ranges::range_value_t<R>` is `T`.
The reference type is implementation-defined: implementations may return
`const T&` into stored elements or `T` by value for handle-based types. Each
collection class exposes its return type as a named member typedef (e.g.
`transaction::input_view`) whose concrete type is *implementation-defined* but
must satisfy *value-range*<T>.

______________________________________________________________________

## Technical Specifications

All additions are relative to the C++ Working Draft.

> *Editorial note — Add `<bitcoin>` to the table of standard library headers in
> [headers] and insert a new Clause [bitcoin] after [time.h.syn].*

______________________________________________________________________

### [bitcoin.version] Feature test macro

```cpp
#define __cpp_lib_bitcoin 202XXXL    // also in <bitcoin>
```

______________________________________________________________________

### [bitcoin.general] General

This Clause describes the header `<bitcoin>`, which provides vocabulary types
for the Bitcoin wire protocol.

______________________________________________________________________

### [bitcoin.syn] Header `<bitcoin>` synopsis

```cpp
namespace bitcoin {

  template<class Tag> class basic-hash-id; // exposition only
  using hash256    = basic-hash-id</* unspecified */>;
  using txid       = basic-hash-id</* unspecified */>;
  using wtxid      = basic-hash-id</* unspecified */>;
  using block_hash = basic-hash-id</* unspecified */>;
  class parse_error;
  class amount;
  class script;
  class outpoint;
  class tx_input;
  class tx_output;
  class transaction;
  class block_header;
  class block;

  namespace literals {
    consteval amount operator""_sat(unsigned long long);
    consteval amount operator""_btc(long double);
    consteval amount operator""_btc(unsigned long long);
  }

} // namespace bitcoin

template<class Tag> struct std::formatter<bitcoin::basic-hash-id<Tag>>;
template<>         struct std::formatter<bitcoin::amount>;

template<class Tag> struct std::hash<bitcoin::basic-hash-id<Tag>>;
template<>         struct std::hash<bitcoin::outpoint>;
```

______________________________________________________________________

### [bitcoin.hashid] Class template `basic-hash-id`

#### [bitcoin.hashid.overview]

`basic-hash-id` is an exposition-only class template. `hash256`
([bitcoin.hash256]), `txid` ([bitcoin.txid]), `wtxid` ([bitcoin.wtxid]),
and `block_hash` ([bitcoin.block_hash]) are distinct specializations with
unspecified tag types. Specializations with different tag types are
unrelated types; comparisons between them are ill-formed.

A default-constructed `basic-hash-id` holds all-zero bytes.

#### [bitcoin.hashid.syn] Synopsis

```cpp
namespace bitcoin {

  template<class Tag>
  class basic-hash-id { // exposition only
  public:
    constexpr basic-hash-id() noexcept;
    constexpr explicit basic-hash-id(std::span<const std::byte, 32> bytes) noexcept;

    [[nodiscard]] constexpr explicit operator bool() const noexcept;

    friend constexpr std::span<const std::byte, 32> as_bytes(const basic-hash-id&)                        noexcept;
    friend constexpr bool                           operator==(const basic-hash-id&, const basic-hash-id&) noexcept = default;
    friend constexpr std::strong_ordering           operator<=>(const basic-hash-id&, const basic-hash-id&) noexcept = default;

  private:
    std::array<std::byte, 32> value_; // exposition only
  };

} // namespace bitcoin

template<class Tag> struct std::formatter<bitcoin::basic-hash-id<Tag>>;
template<class Tag> struct std::hash<bitcoin::basic-hash-id<Tag>>;
```

#### [bitcoin.hashid.cons] Constructors

```cpp
constexpr basic-hash-id() noexcept;
```

*Postconditions:* `!*this` is `true`.

```cpp
constexpr explicit basic-hash-id(std::span<const std::byte, 32> bytes) noexcept;
```

*Effects:* Initializes the stored bytes by copying from `bytes`.

#### [bitcoin.hashid.obs] Observers

```cpp
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

*Returns:* `false` if all stored bytes are `std::byte{0}`, and `true` otherwise.

```cpp
friend constexpr std::span<const std::byte, 32> as_bytes(const basic-hash-id& h) noexcept;
```

*Returns:* A read-only view of the 32 wire-order bytes of `h`.

#### [bitcoin.hashid.fmt] Formatter

`std::formatter<bitcoin::basic-hash-id<Tag>>` formats a value as 64 lowercase
hexadecimal digits in display byte order (bytes reversed relative to wire
order), as used by block explorers.

#### [bitcoin.hashid.hash] Hash support

`std::hash<bitcoin::basic-hash-id<Tag>>` is provided. The hash value is
computed over the 32 wire-order bytes of the value.

______________________________________________________________________

### [bitcoin.hash256] Type `hash256`

`hash256` is a general-purpose 32-byte hash value, used where no stronger
domain type applies — for example, the merkle root in a block header
([bitcoin.block_header]).

#### [bitcoin.hash256.syn] Synopsis

```cpp
namespace bitcoin {

  using hash256 = basic-hash-id</* unspecified */>;

} // namespace bitcoin
```

______________________________________________________________________

### [bitcoin.txid] Type `txid`

`txid` identifies a transaction by the SHA256d of its witness-stripped
serialization.

#### [bitcoin.txid.syn] Synopsis

```cpp
namespace bitcoin {

  using txid = basic-hash-id</* unspecified */>;

} // namespace bitcoin
```

______________________________________________________________________

### [bitcoin.wtxid] Type `wtxid`

`wtxid` is the SHA256d of the full transaction serialization including witness
data, as defined by BIP-141. A `wtxid` and the `txid` of the same transaction
are equal only for transactions that carry no witness data.

#### [bitcoin.wtxid.syn] Synopsis

```cpp
namespace bitcoin {

  using wtxid = basic-hash-id</* unspecified */>;

} // namespace bitcoin
```

______________________________________________________________________

### [bitcoin.block_hash] Type `block_hash`

`block_hash` is the SHA256d of the serialized block header fields.

#### [bitcoin.block_hash.syn] Synopsis

```cpp
namespace bitcoin {

  using block_hash = basic-hash-id</* unspecified */>;

} // namespace bitcoin
```

______________________________________________________________________

### [bitcoin.parse_error] Class `parse_error`

#### [bitcoin.parse_error.syn] Synopsis

```cpp
namespace bitcoin {

  class parse_error : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
  };

} // namespace bitcoin
```

`parse_error` is thrown by the deserializing constructors of `transaction`
([bitcoin.transaction.cons]) and `block` ([bitcoin.block.cons]) when `raw`
does not contain a valid, complete wire-format encoding.

______________________________________________________________________

### [bitcoin.amount] Class `amount`

#### [bitcoin.amount.overview]

An `amount` holds a satoshi-denominated monetary value. The invariant
`0 <= satoshis() <= 2'100'000'000'000'000` holds for every `amount` object.

#### [bitcoin.amount.syn] Synopsis

```cpp
namespace bitcoin {

  class amount {
  public:
    static constexpr amount zero()      noexcept;
    static constexpr amount coin()      noexcept;
    static constexpr amount max_money() noexcept;

    constexpr amount() noexcept;
    constexpr explicit amount(std::int64_t sat);

    [[nodiscard]] constexpr std::int64_t satoshis() const noexcept;

    constexpr amount  operator+ (amount rhs) const;
    constexpr amount  operator- (amount rhs) const;
    constexpr amount  operator* (std::int64_t scalar) const;
    constexpr amount  operator/ (std::int64_t divisor) const;
    constexpr amount& operator+=(amount rhs);
    constexpr amount& operator-=(amount rhs);

    friend bool operator==(amount, amount) noexcept = default;
    friend auto operator<=>(amount, amount) noexcept = default;

  private:
    std::int64_t n; // exposition only
  };

} // namespace bitcoin

template<> struct std::formatter<bitcoin::amount>;
```

#### [bitcoin.amount.static] Static members

```cpp
static constexpr amount zero()      noexcept; // returns amount{}
static constexpr amount coin()      noexcept; // returns amount{100'000'000}
static constexpr amount max_money() noexcept; // returns amount{2'100'000'000'000'000}
```

#### [bitcoin.amount.cons] Constructors

```cpp
constexpr amount() noexcept;
```

*Postconditions:* `satoshis() == 0`.

```cpp
constexpr explicit amount(std::int64_t sat);
```

*Preconditions:* `sat >= 0 && sat <= 2'100'000'000'000'000`.

*Effects:* Initializes the stored value to `sat`.

*Throws:* `std::out_of_range` if the precondition is not satisfied.

#### [bitcoin.amount.obs] Observers

```cpp
[[nodiscard]] constexpr std::int64_t satoshis() const noexcept;
```

*Returns:* The stored satoshi value.

#### [bitcoin.amount.arith] Arithmetic

```cpp
constexpr amount  operator+(amount rhs) const;
constexpr amount& operator+=(amount rhs);
```

*Returns / Effects:* The sum of `*this` and `rhs`.

*Throws:* `std::overflow_error` if the result would exceed `max_money()`.

```cpp
constexpr amount  operator-(amount rhs) const;
constexpr amount& operator-=(amount rhs);
```

*Returns / Effects:* The difference `*this - rhs`.

*Throws:* `std::overflow_error` if the result would be negative.

```cpp
constexpr amount operator*(std::int64_t scalar) const;
```

*Returns:* The product of `satoshis()` and `scalar`.

*Throws:* `std::overflow_error` if the result would violate the class invariant.

```cpp
constexpr amount operator/(std::int64_t divisor) const;
```

*Preconditions:* `divisor != 0`.

*Returns:* `satoshis()` divided by `divisor`, truncated toward zero.

*Throws:* `std::invalid_argument` if `divisor == 0`.

#### [bitcoin.amount.fmt] Formatter

`std::formatter<bitcoin::amount>` supports two format specifiers:
- Default (empty spec): `"X.XXXXXXXX BTC"` with exactly eight decimal places.
- `"sat"`: the raw satoshi count as a decimal integer.

______________________________________________________________________

### [bitcoin.literals] Literals

```cpp
consteval bitcoin::amount operator""_sat(unsigned long long v);
```

*Constraints:* `v <= 2'100'000'000'000'000`.

*Returns:* `bitcoin::amount{static_cast<std::int64_t>(v)}`.

```cpp
consteval bitcoin::amount operator""_btc(unsigned long long v);
```

*Constraints:* `v * 100'000'000 <= 2'100'000'000'000'000`.

*Returns:* `bitcoin::amount{static_cast<std::int64_t>(v) * 100'000'000}`.

```cpp
consteval bitcoin::amount operator""_btc(long double v);
```

*Constraints:* The nearest representable satoshi value of `v * 100'000'000` lies
in `[0, 2'100'000'000'000'000]`.

*Returns:* `bitcoin::amount{static_cast<std::int64_t>(std::roundl(v * 100'000'000L))}`.

______________________________________________________________________

### [bitcoin.script] Class `script`

#### [bitcoin.script.syn] Synopsis

```cpp
namespace bitcoin {

  class script {
  public:
    script() noexcept;
    explicit script(std::span<const std::byte> b);

    friend std::span<const std::byte> as_bytes(const script& s) noexcept;
    friend bool operator==(const script& lhs, const script& rhs) noexcept;
  };

} // namespace bitcoin
```

#### [bitcoin.script.cons] Constructors

```cpp
script() noexcept;
```

*Postconditions:* `as_bytes(*this).empty()` is `true`.

```cpp
explicit script(std::span<const std::byte> b);
```

*Preconditions:* `b.size() <= 10'000`.

*Effects:* Initializes the stored byte sequence by copying from `b`.

*Throws:* `std::length_error` if `b.size() > 10'000`.

#### [bitcoin.script.obs] Hidden friends

```cpp
[[nodiscard]] friend std::span<const std::byte> as_bytes(const script& s) noexcept;
```

*Returns:* A span over the stored byte sequence of `s`.

______________________________________________________________________

### [bitcoin.outpoint] Class `outpoint`

#### [bitcoin.outpoint.syn] Synopsis

```cpp
namespace bitcoin {

  class outpoint {
  public:
    [[nodiscard]] bitcoin::txid txid()  const noexcept;
    [[nodiscard]] std::size_t   index() const noexcept;

    friend bool                 operator==(const outpoint& lhs, const outpoint& rhs) noexcept;
    friend std::strong_ordering operator<=>(const outpoint& lhs, const outpoint& rhs) noexcept;
  };

} // namespace bitcoin

template<> struct std::hash<bitcoin::outpoint>;
```

#### [bitcoin.outpoint.hash] Hash support

`std::hash<bitcoin::outpoint>` is provided. The hash value is computed by
combining `std::hash<bitcoin::txid>{}(txid())` with `index()` in an
implementation-defined manner consistent with `operator==`.

#### [bitcoin.outpoint.obs] Observers

```cpp
[[nodiscard]] bitcoin::txid txid()  const noexcept;
[[nodiscard]] std::size_t   index() const noexcept;
```

*Returns:* The `txid` of the transaction containing the referenced output, and
the output index within that transaction, respectively.

______________________________________________________________________

### [bitcoin.tx_input] Class `tx_input`

#### [bitcoin.tx_input.overview]

`witness_view` satisfies the *value-range*<`std::span<const std::byte>`> named
requirement. Construction is implementation-defined.

#### [bitcoin.tx_input.syn] Synopsis

```cpp
namespace bitcoin {

  class tx_input {
  public:
    using witness_view = /* see [bitcoin.tx_input.overview] */;

    [[nodiscard]] const bitcoin::outpoint& previous_output() const noexcept;
    [[nodiscard]] const bitcoin::script&   script()          const noexcept;
    [[nodiscard]] std::uint32_t            sequence()        const noexcept;
    [[nodiscard]] witness_view             witness()         const noexcept;

    friend bool operator==(const tx_input& lhs, const tx_input& rhs) noexcept;
  };

} // namespace bitcoin
```

#### [bitcoin.tx_input.obs] Observers

```cpp
[[nodiscard]] const bitcoin::outpoint& previous_output() const noexcept;
[[nodiscard]] const bitcoin::script&   script()          const noexcept;
[[nodiscard]] std::uint32_t            sequence()        const noexcept;
```

*Returns:* The previous output reference, input script, and sequence
number, respectively.

```cpp
[[nodiscard]] witness_view witness() const noexcept;
```

*Returns:* A view of the witness items for this input. Each element is a
`std::span<const std::byte>` over one witness item. An empty view indicates
a non-SegWit input.

______________________________________________________________________

### [bitcoin.tx_output] Class `tx_output`

#### [bitcoin.tx_output.syn] Synopsis

```cpp
namespace bitcoin {

  class tx_output {
  public:
    [[nodiscard]] bitcoin::amount        value()  const noexcept;
    [[nodiscard]] const bitcoin::script& script() const noexcept;

    friend bool operator==(const tx_output& lhs, const tx_output& rhs) noexcept;
  };

} // namespace bitcoin
```

#### [bitcoin.tx_output.obs] Observers

```cpp
[[nodiscard]] bitcoin::amount        value()         const noexcept;
[[nodiscard]] const bitcoin::script& script()        const noexcept;
```

*Returns:* The stored output value and output script, respectively.

______________________________________________________________________

### [bitcoin.transaction] Class `transaction`

#### [bitcoin.transaction.overview]

`input_view` and `output_view` each satisfy the *value-range*<T> named
requirement for their respective element types. Objects of type `transaction`
may be default-constructed or deserialized from raw bytes
([bitcoin.transaction.cons]). All other construction is implementation-defined.

#### [bitcoin.transaction.syn] Synopsis

```cpp
namespace bitcoin {

  class transaction {
  public:
    using input_view  = /* see [bitcoin.transaction.overview] */;
    using output_view = /* see [bitcoin.transaction.overview] */;

    transaction() noexcept;
    explicit transaction(std::span<const std::byte> raw);

    [[nodiscard]] bitcoin::txid  id()         const noexcept;
    [[nodiscard]] bitcoin::wtxid witness_id() const noexcept;
    [[nodiscard]] std::int32_t   version()    const noexcept;
    [[nodiscard]] std::uint32_t  locktime()   const noexcept;
    [[nodiscard]] input_view     inputs()     const noexcept;
    [[nodiscard]] output_view    outputs()    const noexcept;

    friend bool operator==(const transaction& lhs, const transaction& rhs) noexcept;
  };

} // namespace bitcoin
```

#### [bitcoin.transaction.cons] Constructors

```cpp
transaction() noexcept;
```

*Postconditions:* `inputs().empty()` and `outputs().empty()`.

```cpp
explicit transaction(std::span<const std::byte> raw);
```

*Effects:* Initializes `*this` by deserializing the Bitcoin wire-format
encoding in `raw`. Both the legacy format and the segwit extended format
(BIP-141/BIP-144) are accepted.

*Throws:* `bitcoin::parse_error` if `raw` does not contain a valid, complete
wire-format encoding of exactly one transaction. In particular, `parse_error`
is thrown if `raw` is truncated or if `raw` contains trailing bytes beyond the
encoding.

______________________________________________________________________

#### [bitcoin.transaction.obs] Observers

```cpp
[[nodiscard]] bitcoin::txid  id()         const noexcept;
[[nodiscard]] bitcoin::wtxid witness_id() const noexcept;
```

*Returns:* The SHA256d hash of the witness-stripped serialization and of the
full serialization including witness data, respectively.

*Remarks:* For a transaction with no witness data, `witness_id() == id()`
when both are compared as `bitcoin::hash256` values. The return types remain
distinct regardless.

```cpp
[[nodiscard]] std::int32_t  version()  const noexcept;
[[nodiscard]] std::uint32_t locktime() const noexcept;
```

*Returns:* The transaction version and locktime fields, respectively.

```cpp
[[nodiscard]] input_view  inputs()  const noexcept;
[[nodiscard]] output_view outputs() const noexcept;
```

*Returns:* Views of the inputs and outputs, respectively.

______________________________________________________________________

### [bitcoin.block_header] Class `block_header`

#### [bitcoin.block_header.syn] Synopsis

```cpp
namespace bitcoin {

  class block_header {
  public:
    [[nodiscard]] bitcoin::block_hash      hash()            const noexcept;
    [[nodiscard]] std::int32_t             version()         const noexcept;
    [[nodiscard]] bitcoin::block_hash      prev_block_hash() const noexcept;
    [[nodiscard]] bitcoin::hash256         merkle_root()     const noexcept;
    [[nodiscard]] std::chrono::sys_seconds time()            const noexcept;
    [[nodiscard]] std::uint32_t            bits()            const noexcept;
    [[nodiscard]] std::uint32_t            nonce()           const noexcept;

    friend bool operator==(const block_header& lhs, const block_header& rhs) noexcept;
  };

} // namespace bitcoin
```

#### [bitcoin.block_header.obs] Observers

```cpp
[[nodiscard]] bitcoin::block_hash      hash()            const noexcept;
[[nodiscard]] std::int32_t             version()         const noexcept;
[[nodiscard]] bitcoin::block_hash      prev_block_hash() const noexcept;
[[nodiscard]] bitcoin::hash256         merkle_root()     const noexcept;
[[nodiscard]] std::chrono::sys_seconds time()            const noexcept;
[[nodiscard]] std::uint32_t            bits()            const noexcept;
[[nodiscard]] std::uint32_t            nonce()           const noexcept;
```

*Returns:* `hash()` returns the SHA256d hash of the 80-byte serialized block
header. The remaining observers return the values of their respective header
fields.

______________________________________________________________________

### [bitcoin.block] Class `block`

#### [bitcoin.block.overview]

`transaction_view` satisfies the *value-range*<`bitcoin::transaction`> named
requirement. Objects of type `block` may be default-constructed or deserialized
from raw bytes ([bitcoin.block.cons]). All other construction is
implementation-defined.

#### [bitcoin.block.syn] Synopsis

```cpp
namespace bitcoin {

  class block {
  public:
    using transaction_view = /* see [bitcoin.block.overview] */;

    block() noexcept;
    explicit block(std::span<const std::byte> raw);

    [[nodiscard]] bitcoin::block_hash          hash()         const noexcept;
    [[nodiscard]] const bitcoin::block_header& header()       const noexcept;
    [[nodiscard]] transaction_view             transactions() const noexcept;

    friend bool operator==(const block& lhs, const block& rhs) noexcept;
  };

} // namespace bitcoin
```

#### [bitcoin.block.cons] Constructors

```cpp
block() noexcept;
```

*Postconditions:* `transactions().empty()`.

```cpp
explicit block(std::span<const std::byte> raw);
```

*Effects:* Initializes `*this` by deserializing the Bitcoin wire-format
encoding in `raw`. The format is an 80-byte block header followed by a
variable-length integer encoding the transaction count, followed by the
serialized transactions in order.

*Throws:* `bitcoin::parse_error` if `raw` does not contain a valid, complete
wire-format encoding of exactly one block. In particular, `parse_error` is
thrown if `raw` is truncated or if `raw` contains trailing bytes beyond the
encoding.

______________________________________________________________________

#### [bitcoin.block.obs] Observers

```cpp
[[nodiscard]] bitcoin::block_hash hash() const noexcept;
```

*Returns:* `header().hash()`.

```cpp
[[nodiscard]] const bitcoin::block_header& header() const noexcept;
```

*Returns:* The block header.

```cpp
[[nodiscard]] transaction_view transactions() const noexcept;
```

*Returns:* A view of the transactions. By convention,
`transactions().front()` is the coinbase transaction when the block is non-empty.

______________________________________________________________________

## References

- **BIP-141** — Segregated Witness (Consensus layer)\
  <https://github.com/bitcoin/bips/blob/master/bip-0141.mediawiki>

- **Bitcoin Developer Reference**\
  <https://developer.bitcoin.org/reference/>

- **Bitcoin Core source** — `src/primitives/`\
  <https://github.com/bitcoin/bitcoin>
