---
title: "Retired: Bitcoin Protocol Predicates"
date: today
document: PREDICATES
audience:
  - Library Evolution Working Group
  - SG14 (Low-Latency / Financial)
author:
  - name: Daniel Pfeifer
    email: <daniel@pfeifer-mail.de>
toc: false
references:
  - id: CPO
    citation-label: CPO
    title: "Bitcoin Customization Point Objects"
    URL: https://purplekarrot.github.io/std-bitcoin/CPO.html
---

[For illustrative purposes only. This document is written in the style of a WG21
standardization paper. It has not been submitted to the ISO C++ committee and is
not under active consideration for standardization.]{.draftnote}

# Status

This paper is retired.

The previous proposal to standardize a collection of free-function protocol
predicates has been withdrawn.

The retained semantic queries are now specified as customization point objects
in [@CPO]:

- `value`
- `output_script`
- `funding_height`
- `is_coinbase`
- `has_witness`

In particular, the former overload set centered on `is_coinbase` and the
additional one-off helpers for sequence numbers, locktimes, output
unspendability, and block shape are no longer proposed for standardization.
