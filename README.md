# Eratosthenes
Fast prime generation for Python based on Sieve of Eratosthenes

## Usage

```python
from eratosthenes import sieve, count, segmented_sieve, segmented_count

# This is a Sieve of Eratosthenes with wheel factorization.
primes = sieve(1000)

# This uses the same logic to count primes up the argument.
num_primes = count(1000)

# This segmented sieve only uses up to 128 KB main memory
primes = segmented_sieve(1000)
num_primes = segmented_count(1000)
```

## About
Eratosthenes is written in C++17 and bound for Python with `pybind11`. This makes it faster than just about any native Python implementation of Sieve of Eratosthenes!

## Copyright

Copyright (c) Daniel Strano and the Qrack contributors 2017-2024. All rights reserved.

Eratosthenes is provided under an MIT License.
