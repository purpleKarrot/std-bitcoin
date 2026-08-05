---
title: Bitcoin Customization Point Objects
date: today
document: CPO
audience:
  - Library Evolution Working Group
  - SG14 (Low-Latency / Financial)
author:
  - name: Daniel Pfeifer
    email: <daniel@pfeifer-mail.de>
toc: false
references:
  - id: VOCABULARY
    citation-label: VOCABULARY
    title: "Bitcoin Vocabulary Types"
    URL: https://purplekarrot.github.io/std-bitcoin/VOCABULARY.html
  - id: BIP141
    citation-label: BIP-141
    title: "BIP-141: Segregated Witness (Consensus layer)"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0141.mediawiki
---

[For illustrative purposes only. This document is written in the style of a WG21
standardization paper. It has not been submitted to the ISO C++ committee and is
not under active consideration for standardization.]{.draftnote}

# Abstract

This paper proposes five customization point objects in `namespace bitcoin`:
`value`, `output_script`, `funding_height`, `is_coinbase`, and `has_witness`.

These names provide a uniform call surface for generic code while permitting
customization by implementation-defined types through ordinary member functions
or unqualified functions found by argument-dependent lookup.

# Motivation

The vocabulary paper [@VOCABULARY] standardizes concrete Bitcoin protocol data
structures. Generic library interfaces, however, sometimes need to query a small
set of properties without fixing the participating type.

The operations in this paper are purely observational. They are therefore a
natural fit for customization point objects rather than for additional
vocabulary types or a growing family of unrelated free functions.

# Impact on the Standard

This paper adds five customization point objects to `namespace bitcoin` inside
`<bitcoin>`. It depends on [@VOCABULARY] and introduces no new user-visible
classes.

# Design considerations

## Member-first, ADL-second lookup

Each CPO first tries a corresponding member operation and otherwise falls back
to an unqualified call found by argument-dependent lookup.

This permits direct implementations on library types while allowing
implementation-defined types to participate without modifying
`namespace bitcoin`.

## Observational semantics

The semantics in this paper are stated in terms of observable results, not in
terms of how those results are computed. An implementation may therefore answer
a query from stored or precomputed state whenever doing so is observationally
equivalent.

# Proposed wording

The wording in this section is relative to the C++ Working Draft and assumes
that the wording of [@VOCABULARY] has been applied.

[Add the following declarations to the `<bitcoin>` header synopsis in
[bitcoin.syn].]{.ednote}

## [bitcoin.cpo.version] Feature test macro

```cpp
#define __cpp_lib_bitcoin_cpo 214XXXL // also in <bitcoin>
```

## [bitcoin.cpo] Bitcoin customization point objects

### [bitcoin.cpo.syn] Synopsis

```cpp
namespace bitcoin::inline /* $unspecified$ */ {

  inline constexpr /* $unspecified$ */ value = /* $unspecified$ */;
  inline constexpr /* $unspecified$ */ output_script = /* $unspecified$ */;
  inline constexpr /* $unspecified$ */ funding_height = /* $unspecified$ */;
  inline constexpr /* $unspecified$ */ is_coinbase = /* $unspecified$ */;
  inline constexpr /* $unspecified$ */ has_witness = /* $unspecified$ */;

}
```

These names denote customization point objects. The effect of calling a
customization point object is described in the corresponding subclause.

### [bitcoin.cpo.value] `value`

`value(x)` is expression-equivalent to:

- `x.value()` if that expression is well-formed;
- otherwise, an unqualified call `value(x)` that does not include the
  declaration of the customization point object `bitcoin::value` in the lookup
  set, if that expression is well-formed;
- otherwise, the expression is ill-formed.

*Remarks:* For the standard-library `tx_output` type, `value(x)` returns the
Satoshi-denominated amount represented by `x`.

### [bitcoin.cpo.output.script] `output_script`

`output_script(x)` is expression-equivalent to:

- `x.output_script()` if that expression is well-formed;
- otherwise, an unqualified call `output_script(x)` that does not include the
  declaration of the customization point object `bitcoin::output_script` in the
  lookup set, if that expression is well-formed;
- otherwise, the expression is ill-formed.

*Remarks:* For the standard-library `tx_output` type, `output_script(x)` returns
a script representing the locking script of `x`.

### [bitcoin.cpo.funding.height] `funding_height`

`funding_height(x)` is expression-equivalent to:

- `x.funding_height()` if that expression is well-formed;
- otherwise, an unqualified call `funding_height(x)` that does not include the
  declaration of the customization point object `bitcoin::funding_height` in the
  lookup set, if that expression is well-formed;
- otherwise, the expression is ill-formed.

*Remarks:* No standard Bitcoin vocabulary type is required to provide a
customization for `funding_height`.

### [bitcoin.cpo.is.coinbase] `is_coinbase`

`is_coinbase(x)` is expression-equivalent to:

- `x.is_coinbase()` if that expression is well-formed;
- otherwise, an unqualified call `is_coinbase(x)` that does not include the
  declaration of the customization point object `bitcoin::is_coinbase` in the
  lookup set, if that expression is well-formed;
- otherwise, the expression is ill-formed.

For the standard-library `transaction` type, `is_coinbase(t)` returns `true` if
`t` has exactly one input and that input's previous-output reference has an
all-zero transaction identifier and an output index equal to `0xFFFF'FFFF`;
otherwise `false`.

### [bitcoin.cpo.has.witness] `has_witness`

`has_witness(x)` is expression-equivalent to:

- `x.has_witness()` if that expression is well-formed;
- otherwise, an unqualified call `has_witness(x)` that does not include the
  declaration of the customization point object `bitcoin::has_witness` in the
  lookup set, if that expression is well-formed;
- otherwise, the expression is ill-formed.

For the standard-library `tx_input` type, `has_witness(i)` returns `true` if
`i.witness()` is non-empty; otherwise `false`.

For the standard-library `transaction` type, `has_witness(t)` returns `true` if
`has_witness(i)` is `true` for any input `i` in `t.inputs()`; otherwise `false`.

*Remarks:* Reflects segregated witness as specified by [@BIP141].
