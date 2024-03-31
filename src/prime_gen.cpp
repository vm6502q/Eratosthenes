// Source: https://www.geeksforgeeks.org/sieve-of-eratosthenes/
// C++ program to print all primes smaller than or equal to
// n using Sieve of Eratosthenes

// Improved by Dan Strano of Unitary Fund, 2024.
// We can think of trial division as exact inverse of
// Sieve of Eratosthenes, with log space and log time.
// The modular division part is a costly atomic operation.
// It need only be carried out up the square root of the
// number under trial. Multiples of 2, 3, 5, 7, and 11 can
// be entirely skipped in loop enumeration.

#include "prime_generator.hpp"
#include "dispatchqueue.hpp"

#include <cmath>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace qimcifa {
DispatchQueue dispatch(std::thread::hardware_concurrency());

std::vector<BigInteger> SieveOfEratosthenes(const BigInteger& n)
{
    std::vector<BigInteger> knownPrimes = { 2U, 3U, 5U, 7U };
    if (n < 2U) {
        return std::vector<BigInteger>();
    }

    if (n < (knownPrimes.back() + 2U)) {
        const auto highestPrimeIt = std::upper_bound(knownPrimes.begin(), knownPrimes.end(), n);
        return std::vector<BigInteger>(knownPrimes.begin(), highestPrimeIt);
    }

    knownPrimes.reserve(std::expint(log(n)) - std::expint(log(2)));

    // We are excluding multiples of the first few
    // small primes from outset. For multiples of
    // 2, 3, and 5 this reduces complexity to 4/15.
    const size_t cardinality = backward5(n);

    // Create a boolean array "prime[0..cardinality]"
    // and initialize all entries it as true. Rather,
    // reverse the true/false meaning, so we can use
    // default initialization. A value in notPrime[i]
    // will finally be false only if i is a prime.
    std::unique_ptr<bool> uNotPrime(new bool[cardinality + 1U]());
    bool* notPrime = uNotPrime.get();

    // We dispatch multiple marking asynchronously.
    // If we've already marked all primes up to x,
    // we're free to continue to up to x * x,
    // then we synchronize.
    BigInteger threadBoundary = 36U;

    // Get the remaining prime numbers.
    unsigned short wheel5 = 129U;
    unsigned long long wheel7 = 9009416540524545ULL;
    size_t o = 1U;
    for (;;) {
        o += GetWheel5and7Increment(wheel5, wheel7);

        const BigInteger p = forward(o);
        if ((p * p) > n) {
            break;
        }

        if (threadBoundary < p) {
            dispatch.finish();
            threadBoundary *= threadBoundary;
        }

        if (notPrime[backward5(p)]) {
            continue;
        }

        knownPrimes.push_back(p);

        dispatch.dispatch([&n, p, &notPrime]() {
            // We are skipping multiples of 2, 3, and 5
            // for space complexity, for 4/15 the bits.
            // More are skipped by the wheel for time.
            const BigInteger p2 = p << 1U;
            const BigInteger p4 = p << 2U;
            BigInteger i = p * p;

            // "p" already definitely not a multiple of 3.
            // Its remainder when divided by 3 can be 1 or 2.
            // If it is 2, we can do a "half iteration" of the
            // loop that would handle remainder of 1, and then
            // we can proceed with the 1 remainder loop.
            // This saves 2/3 of updates (or modulo).
            if ((p % 3U) == 2U) {
                notPrime[backward5(i)] = true;
                i += p2;
                if (i > n) {
                    return false;
                }
            }

            for (;;) {
                if (i % 5U) {
                    notPrime[backward5(i)] = true;
                }
                i += p4;
                if (i > n) {
                    return false;
                }

                if (i % 5U) {
                    notPrime[backward5(i)] = true;
                }
                i += p2;
                if (i > n) {
                    return false;
                }
            }

            return false;
        });
    }

    dispatch.finish();

    for (;;) {
        const BigInteger p = forward(o);
        if (p > n) {
            break;
        }

        o += GetWheel5and7Increment(wheel5, wheel7);

        if (notPrime[backward5(p)]) {
            continue;
        }

        knownPrimes.push_back(p);
    }

    return knownPrimes;
}

// Pardon the obvious "copy/pasta."
// I began to design a single method to switch off between these two,
// then I realized the execution time overhead of the implementation.
// (It would compound linearly over the cardinality to check.)
// It is certainly "cheap" to copy/paste, but that's our only goal.

BigInteger CountPrimesTo(const BigInteger& n)
{
    constexpr BigInteger knownPrimes[4U] = { 2U, 3U, 5U, 7U };
    if (n < 2U) {
        return 0U;
    }

    if (n < 11U) {
        const auto highestPrimeIt = std::upper_bound(knownPrimes, knownPrimes + 4U, n);
        return std::distance(knownPrimes, highestPrimeIt);
    }

    // We are excluding multiples of the first few
    // small primes from outset. For multiples of
    // 2, 3, and 5 this reduces complexity to 4/15.
    const size_t cardinality = backward5(n);

    // Create a boolean array "prime[0..cardinality]"
    // and initialize all entries it as true. Rather,
    // reverse the true/false meaning, so we can use
    // default initialization. A value in notPrime[i]
    // will finally be false only if i is a prime.
    std::unique_ptr<bool> uNotPrime(new bool[cardinality + 1U]());
    bool* notPrime = uNotPrime.get();

    // We dispatch multiple marking asynchronously.
    // If we've already marked all primes up to x,
    // we're free to continue to up to x * x,
    // then we synchronize.
    BigInteger threadBoundary = 36U;

    // Get the remaining prime numbers.
    unsigned short wheel5 = 129U;
    unsigned long long wheel7 = 9009416540524545ULL;
    size_t o = 1U;
    BigInteger count = 4U;
    for (;;) {
        o += GetWheel5and7Increment(wheel5, wheel7);

        const BigInteger p = forward(o);
        if ((p * p) > n) {
            break;
        }

        if (threadBoundary < p) {
            dispatch.finish();
            threadBoundary *= threadBoundary;
        }

        if (notPrime[backward5(p)]) {
            continue;
        }

        ++count;

        dispatch.dispatch([&n, p, &notPrime]() {
            // We are skipping multiples of 2, 3, and 5
            // for space complexity, for 4/15 the bits.
            // More are skipped by the wheel for time.
            const BigInteger p2 = p << 1U;
            const BigInteger p4 = p << 2U;
            BigInteger i = p * p;

            // "p" already definitely not a multiple of 3.
            // Its remainder when divided by 3 can be 1 or 2.
            // If it is 2, we can do a "half iteration" of the
            // loop that would handle remainder of 1, and then
            // we can proceed with the 1 remainder loop.
            // This saves 2/3 of updates (or modulo).
            if ((p % 3U) == 2U) {
                notPrime[backward5(i)] = true;
                i += p2;
                if (i > n) {
                    return false;
                }
            }

            for (;;) {
                if (i % 5U) {
                    notPrime[backward5(i)] = true;
                }
                i += p4;
                if (i > n) {
                    return false;
                }

                if (i % 5U) {
                    notPrime[backward5(i)] = true;
                }
                i += p2;
                if (i > n) {
                    return false;
                }
            }

            return false;
        });
    }

    dispatch.finish();

    for (;;) {
        const BigInteger p = forward(o);
        if (p > n) {
            break;
        }

        o += GetWheel5and7Increment(wheel5, wheel7);

        if (notPrime[backward5(p)]) {
            continue;
        }

        ++count;
    }

    return count;
}

std::vector<BigInteger> SegmentedSieveOfEratosthenes(BigInteger n)
{
    // TODO: This should scale to the system.
    // Assume the L1/L2 cache limit is 2048 KB.
    // We save half our necessary bytes by
    // removing multiples of 2.
    // The simple sieve removes multiples of 2, 3, and 5.
    // limit = 2048 KB = 2097152 B,
    // limit_segmented = limit * 2
    // limit_simple = ((((limit * 2) * 3) / 2) * 5) / 4
    constexpr size_t limit = 4194304ULL;
    constexpr size_t limit_simple = 31457281ULL;

    if (!(n & 1U)) {
        --n;
    }
    if (limit_simple >= n) {
        return SieveOfEratosthenes(n);
    }
    std::vector<BigInteger> knownPrimes = SieveOfEratosthenes(limit_simple);
    knownPrimes.reserve(std::expint(log(n)) - std::expint(log(2)));

    // Divide the range in different segments
    const size_t nCardinality = backward2(n);
    size_t low = backward2(limit_simple);
    size_t high = low + limit;

    // Process one segment at a time till we pass n.
    while (low < nCardinality)
    {
        if (high > nCardinality) {
           high = nCardinality;
        }

        const BigInteger fLo = forward2(low);
        const size_t sqrtIndex = std::distance(
            knownPrimes.begin(),
            std::upper_bound(knownPrimes.begin(), knownPrimes.end(), sqrt(forward2(high)) + 1U)
        );

        const size_t cardinality = high - low;
        bool notPrime[cardinality + 1U] = { false };

        for (size_t k = 1U; k < sqrtIndex; ++k) {
            const BigInteger& p = knownPrimes[k];
            dispatch.dispatch([&fLo, &low, &cardinality, p, &notPrime]() {
                // We are skipping multiples of 2.
                const BigInteger p2 = p << 1U;

                // Find the minimum number in [low..high] that is
                // a multiple of prime[i] (divisible by prime[i])
                // For example, if low is 31 and prime[i] is 3,
                // we start with 33.
                BigInteger i = (fLo / p) * p;
                if (i < fLo) {
                    i += p;
                }
                if ((i & 1U) == 0U) {
                    i += p;
                }

                for (;;) {
                    const size_t o = backward2(i) - low;
                    if (o > cardinality) {
                        return false;
                    }
                    notPrime[o] = true;
                    i += p2;
                }

                return false;
            });
        }
        dispatch.finish();

        // Numbers which are not marked are prime
        for (size_t o = 1U; o <= cardinality; ++o) {
            if (!notPrime[o]) {
                knownPrimes.push_back(forward2(o + low));
            }
        }

        // Update low and high for next segment
        low = low + limit;
        high = low + limit;
    }

    return knownPrimes;
}

BigInteger SegmentedCountPrimesTo(BigInteger n)
{
    // TODO: This should scale to the system.
    // Assume the L1/L2 cache limit is 2048 KB.
    // We save half our necessary bytes by
    // removing multiples of 2.
    // The simple sieve removes multiples of 2, 3, and 5.
    // limit = 2048 KB = 2097152 B,
    // limit_segmented = limit * 2
    // limit_simple = ((((limit * 2) * 3) / 2) * 5) / 4
    constexpr size_t limit = 4194304ULL;
    constexpr size_t limit_simple = 31457281ULL;

    if (!(n & 1U)) {
        --n;
    }
    if (limit_simple >= n) {
        return CountPrimesTo(n);
    }
    const BigInteger sqrtnp1 = sqrt(n) + 1U;
    const BigInteger practicalLimit = ((sqrtnp1 < limit_simple) ? sqrtnp1 : limit_simple) | 1U;
    std::vector<BigInteger> knownPrimes = SieveOfEratosthenes(practicalLimit);
    if (practicalLimit < sqrtnp1) {
        knownPrimes.reserve(std::expint(log(sqrtnp1)) - std::expint(log(2)));
    }
    size_t count = knownPrimes.size();

    // Divide the range in different segments
    const size_t nCardinality = backward2(n);
    size_t low = backward2(practicalLimit);
    size_t high = low + limit;

    // Process one segment at a time till we pass n.
    while (low < nCardinality)
    {
        if (high > nCardinality) {
           high = nCardinality;
        }
        const BigInteger fLo = forward2(low);
        const size_t sqrtIndex = std::distance(
            knownPrimes.begin(),
            std::upper_bound(knownPrimes.begin(), knownPrimes.end(), sqrt(forward2(high)) + 1U)
        );

        const size_t cardinality = high - low;
        bool notPrime[cardinality + 1U] = { false };

        // Use the primes found by the simple sieve
        // to find primes in current range
        for (size_t k = 1U; k < sqrtIndex; ++k) {
            const BigInteger& p = knownPrimes[k];
            dispatch.dispatch([&fLo, &low, &cardinality, p, &notPrime]() {
                // We are skipping multiples of 2.
                const BigInteger p2 = p << 1U;

                // Find the minimum number in [low..high] that is
                // a multiple of prime[i] (divisible by prime[i])
                // For example, if low is 31 and prime[i] is 3,
                // we start with 33.
                BigInteger i = (fLo / p) * p;
                if (i < fLo) {
                    i += p;
                }
                if ((i & 1U) == 0U) {
                    i += p;
                }

                for (;;) {
                    const size_t o = backward2(i) - low;
                    if (o > cardinality) {
                        return false;
                    }
                    notPrime[o] = true;
                    i += p2;
                }

                return false;
            });
        }
        dispatch.finish();

        if (knownPrimes.back() > sqrtnp1) {
            for (size_t o = 1U; o <= cardinality; ++o) {
                if (!notPrime[o]) {
                    ++count;
                }
            }
        } else {
            for (size_t o = 1U; o <= cardinality; ++o) {
                if (!notPrime[o]) {
                    const BigInteger p = forward2(o + low);
                    if (p <= sqrtnp1) {
                        knownPrimes.push_back(p);
                    }
                    ++count;
                }
            }
        }

        // Update low and high for next segment
        low = low + limit;
        high = low + limit;
    }

    return count;
}
} // namespace qimcifa

using namespace qimcifa;

PYBIND11_MODULE(eratosthenes, m) {
    m.doc() = "pybind11 plugin to generate prime numbers";
    m.def("count", &CountPrimesTo, "Counts the prime numbers between 1 and the value of its argument");
    m.def("segmented_count", &SegmentedCountPrimesTo, "Counts the primes in capped space complexity");
    m.def("sieve", &SieveOfEratosthenes, "Returns all primes up to the value of its argument (using Sieve of Eratosthenes)");
    m.def("segmented_sieve", &SegmentedSieveOfEratosthenes, "Returns the primes in capped space complexity");
}
