# Eratosthenes
Fast prime generation for Python based on Sieve of Eratosthenes and Trial Division

## Usage

```python
from eratosthenes import sieve, trial_division

# Either of these will produce the prime numbers
# up to the value of their argument.
p1 = sieve(1000)

# Trial division uses virtually no memory, but
# it's much slower.
p2 = trial_division(1000)
```

## About
Eratosthenes is written in C++17 and bound for Python with `pybind11`. This makes it faster than just about any native Python implementation of Sieve of Eratosthenes or Trial Division!

## Copyright

Copyright (c) Daniel Strano and the Qrack contributors 2017-2024. All rights reserved.

Eratosthenes is provided under an MIT License.
