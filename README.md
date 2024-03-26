# Eratosthenes
Fast prime generation for Python based on Sieve of Eratosthenes and Trial Division

## Usage

```python
from eratosthenes import sieve, trial_division

# Any of these will produce the prime numbers
# up to the value of their argument.

# This is a Sieve of Eratosthenes ("SoE"),
# with wheel factorization.
p1 = sieve(1000)

# If memory is limited, then SoE can be limited
# to a segment size. Specify it in bits of RAM.
p1 = segmented_sieve(1000, 1 << 36)

# Trial division uses virtually no memory,
# but it's much slower.
p2 = trial_division(1000)
```

## About
Eratosthenes is written in C++17 and bound for Python with `pybind11`. This makes it faster than just about any native Python implementation of Sieve of Eratosthenes or Trial Division!

## Copyright

Copyright (c) Daniel Strano and the Qrack contributors 2017-2024. All rights reserved.

Eratosthenes is provided under an MIT License.
