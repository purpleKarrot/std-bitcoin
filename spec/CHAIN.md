---
title: "Retired: Bitcoin Chains"
date: today
document: CHAIN
audience:
  - Library Evolution Working Group
  - SG14 (Low-Latency / Financial)
author:
  - name: Daniel Pfeifer
    email: <daniel@pfeifer-mail.de>
toc: false
references:
  - id: VALIDATION
    citation-label: VALIDATION
    title: "Bitcoin Consensus Validation"
    URL: https://purplekarrot.github.io/std-bitcoin/VALIDATION.html
---

[For illustrative purposes only. This document is written in the style of a WG21
standardization paper. It has not been submitted to the ISO C++ committee and is
not under active consideration for standardization.]{.draftnote}

# Status

This paper is retired.

The public `chain_view` abstraction is now specified in [@VALIDATION]. The
previous proposal to standardize a public type-erased wrapper
`bitcoin::any_chain_view` has been withdrawn.

Implementations may still adapt `chain_view` arguments to private non-owning,
type-erased representations internally, but that mechanism is an implementation
detail and is not part of the standardized API.
