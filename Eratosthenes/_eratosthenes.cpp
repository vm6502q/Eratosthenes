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

#include "dispatchqueue.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace qimcifa {

typedef boost::multiprecision::cpp_int BigInteger;

inline BigInteger sqrt(const BigInteger& toTest)
{
    // Otherwise, find b = sqrt(b^2).
    BigInteger start = 1U, end = toTest >> 1U, ans = 0U;
    do {
        const BigInteger mid = (start + end) >> 1U;

        // If toTest is a perfect square
        const BigInteger sqr = mid * mid;
        if (sqr == toTest) {
            return mid;
        }

        if (sqr < toTest) {
            // Since we need floor, we update answer when mid*mid is smaller than p, and move closer to sqrt(p).
            start = mid + 1U;
            ans = mid;
        } else {
            // If mid*mid is greater than p
            end = mid - 1U;
        }
    } while (start <= end);

    return ans;
}

inline BigInteger forward2(const size_t& p) {
    // Make this NOT a multiple of 2.
    return (p << 1U) | 1U;
}

inline size_t backward2(const BigInteger& p) {
    return (size_t)(p >> 1U);
}

inline BigInteger forward3(const size_t& p) {
    // Make this NOT a multiple of 2 or 3.
    return (p << 1U) + (~(~p | 1U)) - 1U;
}

inline size_t backward3(const BigInteger& n) {
    return (size_t)((~(~n | 1U)) / 3U) + 1U;
}

constexpr unsigned char wheel5[8U] = {
    1U, 7U, 11U, 13U, 17U, 19U, 23U, 29U
};

inline BigInteger forward5(const size_t& p) {
    return wheel5[p % 8U] + (p / 8U) * 30U;
}

inline size_t backward5(const BigInteger &n) {
    return (size_t)(std::distance(wheel5, std::lower_bound(wheel5, wheel5 + 8U, n % 30U)) + 8U * (n / 30U) + 1U);
}

constexpr unsigned char wheel7[48U] = {
    1U, 11U, 13U, 17U, 19U, 23U, 29U, 31U, 37U, 41U, 43U, 47U, 53U, 59U, 61U, 67U, 71U, 73U, 79U, 83U, 89U,
    97U, 101U, 103U, 107U, 109U, 113U, 121U, 127U, 131U, 137U, 139U, 143U, 149U, 151U, 157U, 163U, 167U,
    169U, 173U, 179U, 181U, 187U, 191U, 193U, 197U, 199U, 209U
};

inline BigInteger forward7(const size_t& p) {
    return wheel7[p % 48U] + (p / 48U) * 210U;
}

inline size_t backward7(const BigInteger& n) {
    return (size_t)(std::distance(wheel7, std::lower_bound(wheel7, wheel7 + 48U, n % 210U)) + 48U * (n / 210U) + 1U);
}

inline size_t GetWheel5and7Increment(unsigned short& wheel5, unsigned long long& wheel7) {
    constexpr unsigned short wheel5Back = 1U << 9U;
    constexpr unsigned long long wheel7Back = 1ULL << 55U;
    unsigned wheelIncrement = 0U;
    bool is_wheel_multiple = false;
    do {
        is_wheel_multiple = (bool)(wheel5 & 1U);
        wheel5 >>= 1U;
        if (is_wheel_multiple) {
            wheel5 |= wheel5Back;
            ++wheelIncrement;
            continue;
        }

        is_wheel_multiple = (bool)(wheel7 & 1U);
        wheel7 >>= 1U;
        if (is_wheel_multiple) {
            wheel7 |= wheel7Back;
        }
        ++wheelIncrement;
    } while (is_wheel_multiple);

    return (size_t)wheelIncrement;
}

DispatchQueue dispatch(std::thread::hardware_concurrency());

std::vector<BigInteger> SieveOfEratosthenes(const BigInteger& n)
{
    std::vector<BigInteger> knownPrimes = { 2U, 3U, 5U, 7U };
    if (n < 2U) {
        return std::vector<BigInteger>();
    }

    if (n < 11U) {
        const auto highestPrimeIt = std::upper_bound(knownPrimes.begin(), knownPrimes.end(), n);
        return std::vector<BigInteger>(knownPrimes.begin(), highestPrimeIt);
    }

    knownPrimes.reserve(std::expint(log((double)n)) - std::expint(log(2)));

    // We are excluding multiples of the first few
    // small primes from outset. For multiples of
    // 2, 3, and 5 this reduces complexity to 4/15.
    const size_t cardinality = backward7(n);

    // Create a boolean array "prime[0..cardinality]"
    // and initialize all entries it as true. Rather,
    // reverse the true/false meaning, so we can use
    // default initialization. A value in notPrime[i]
    // will finally be false only if i is a prime.
    std::unique_ptr<bool[]> uNotPrime(new bool[cardinality + 1U]());
    bool* notPrime = uNotPrime.get();

    // We dispatch multiple marking asynchronously.
    // If we've already marked all primes up to x,
    // we're free to continue to up to x * x,
    // then we synchronize.
    BigInteger threadBoundary = 81U;

    // Get the remaining prime numbers.
    size_t o = 1U;
    for (;;) {
        const BigInteger p = forward7(o);
        if ((p * p) > n) {
            break;
        }

        if (threadBoundary < p) {
            dispatch.finish();
            threadBoundary *= threadBoundary;
        }

        if (notPrime[o]) {
            continue;
        }

        knownPrimes.push_back(p);

        dispatch.dispatch([&n, p, &notPrime]() {
            for (BigInteger i = p * p; i <= n; i += 11U * p) {
                notPrime[backward7(i)] = true;
            }
            return false;
        });
    }

    dispatch.finish();

    ++o;
    for (;;) {
        const BigInteger p = forward7(o);
        if (p > n) {
            break;
        }

        if (notPrime[o]) {
            continue;
        }

        knownPrimes.push_back(p);
        ++o;
    }

    return knownPrimes;
}

std::vector<std::string> _SieveOfEratosthenes(const std::string& n) {
    std::vector<BigInteger> v = SieveOfEratosthenes(BigInteger(n));
    std::vector<std::string> toRet;
    toRet.reserve(v.size());
    for (const BigInteger& p : v) {
        toRet.push_back(boost::lexical_cast<std::string>(p));
    }

    return toRet;
}

// Pardon the obvious "copy/pasta."
// I began to design a single method to switch off between these two,
// then I realized the execution time overhead of the implementation.
// (It would compound linearly over the cardinality to check.)
// It is certainly "cheap" to copy/paste, but that's our only goal.

BigInteger CountPrimesTo(const BigInteger& n)
{
    const BigInteger knownPrimes[4U] = { 2U, 3U, 5U, 7U };
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
    const size_t cardinality = backward7(n);

    // Create a boolean array "prime[0..cardinality]"
    // and initialize all entries it as true. Rather,
    // reverse the true/false meaning, so we can use
    // default initialization. A value in notPrime[i]
    // will finally be false only if i is a prime.
    std::unique_ptr<bool[]> uNotPrime(new bool[cardinality + 1U]());
    bool* notPrime = uNotPrime.get();

    // We dispatch multiple marking asynchronously.
    // If we've already marked all primes up to x,
    // we're free to continue to up to x * x,
    // then we synchronize.
    BigInteger threadBoundary = 36U;

    // Get the remaining prime numbers.
    size_t o = 1U;
    BigInteger count = 4U;
    for (;;) {
        const BigInteger p = forward7(o);
        if ((p * p) > n) {
            break;
        }

        if (threadBoundary < p) {
            dispatch.finish();
            threadBoundary *= threadBoundary;
        }

        if (notPrime[o]) {
            continue;
        }

        ++count;

        dispatch.dispatch([&n, p, &notPrime]() {
            for (BigInteger i = p * p; i <= n; i += 13U * p) {
                notPrime[backward7(i)] = true;
            }
            return false;
        });
    }

    dispatch.finish();

    ++o;
    for (;;) {
        const BigInteger p = forward7(o);
        if (p > n) {
            break;
        }

        if (notPrime[o]) {
            continue;
        }

        ++count;
        ++o;
    }

    return count;
}

std::string _CountPrimesTo(const std::string& n) {
    return boost::lexical_cast<std::string>(CountPrimesTo(BigInteger(n)));
}

std::vector<BigInteger> SegmentedSieveOfEratosthenes(BigInteger n)
{
    // TODO: This should scale to the system.
    // Assume the L1/L2 cache limit is 2048 KB.
    // We save half our necessary bytes by
    // removing multiples of 2.
    // The simple sieve removes multiples of 2, 3, 5, 7, and 11.
    // limit = 2048 KB = 2097152 B,
    // limit = ((((((limit * 2) * 3) / 2) * 5) / 4) * 7) / 6
    constexpr size_t limit = 9175040ULL;

    if (!(n & 1U)) {
        --n;
    }
    while (!(n % 3U) || !(n % 5U) || !(n % 7U)) {
        n -= 2U;
    }
    if (limit >= n) {
        return SieveOfEratosthenes(n);
    }
    std::vector<BigInteger> knownPrimes = SieveOfEratosthenes(limit);
    knownPrimes.reserve(std::expint(log((double)n)) - std::expint(log(2)));

    // Divide the range in different segments
    const size_t nCardinality = backward7(n);
    size_t low = backward7(limit);
    size_t high = low + limit;

    // Process one segment at a time till we pass n.
    while (low < nCardinality)
    {
        if (high > nCardinality) {
           high = nCardinality;
        }

        const BigInteger fLo = forward7(low);
        const size_t sqrtIndex = std::distance(
            knownPrimes.begin(),
            std::upper_bound(knownPrimes.begin(), knownPrimes.end(), sqrt(forward7(high)) + 1U)
        );

        const size_t cardinality = high - low;
        bool notPrime[cardinality + 1U] = { false };

        for (size_t k = 3U; k < sqrtIndex; ++k) {
            const BigInteger& p = knownPrimes[k];
            dispatch.dispatch([&n, &fLo, p, &notPrime]() {
                // We are skipping multiples of 2.
                const BigInteger p2 = p << 1U;

                // Find the minimum number in [low..high] that is
                // not a multiple of wheel factorization primes.
                BigInteger i = (fLo / p) * p;
                if (i < fLo) {
                    i += p;
                }
                if (!(i & 1U)) {
                    i += p;
                }
                if (!(i % 3U)) {
                    i += p2;
                }
                if (!(i % 5U)) {
                    i += p2;
                }
                if (!(i % 7U)) {
                    i += p2;
                }

                for (; i <= n; i += 11U * p) {
                    notPrime[backward7(i)] = true;
                }
                return false;
            });
        }
        dispatch.finish();

        // Numbers which are not marked are prime
        for (size_t o = 1U; o <= cardinality; ++o) {
            if (!notPrime[o]) {
                knownPrimes.push_back(forward7(o + low));
            }
        }

        // Update low and high for next segment
        low = low + limit;
        high = low + limit;
    }

    return knownPrimes;
}

std::vector<std::string> _SegmentedSieveOfEratosthenes(const std::string& n) {
    std::vector<BigInteger> v = SegmentedSieveOfEratosthenes(BigInteger(n));
    std::vector<std::string> toRet;
    toRet.reserve(v.size());
    for (const BigInteger& p : v) {
        toRet.push_back(boost::lexical_cast<std::string>(p));
    }

    return toRet;
}

BigInteger SegmentedCountPrimesTo(BigInteger n)
{
    // TODO: This should scale to the system.
    // Assume the L1/L2 cache limit is 2048 KB.
    // We save half our necessary bytes by
    // removing multiples of 2.
    // The simple sieve removes multiples of 2, 3, and 5.
    // limit = 2048 KB = 2097152 B,
    // limit = ((((((limit * 2) * 3) / 2) * 5) / 4) * 7) / 6
    constexpr size_t limit = 9175040ULL;

    if (!(n & 1U)) {
        --n;
    }
    while (!(n % 3U) || !(n % 5U) || !(n % 7U)) {
        n -= 2U;
    }
    if (limit >= n) {
        return CountPrimesTo(n);
    }
    BigInteger sqrtnp1 = (sqrt(n) + 1U) | 1U;
    while (!(sqrtnp1 % 3U) || !(sqrtnp1 % 5U) || !(sqrtnp1 % 7U)) {
        sqrtnp1 += 2U;
    }
    const BigInteger practicalLimit = (sqrtnp1 < limit) ? sqrtnp1 : limit;
    std::vector<BigInteger> knownPrimes = SieveOfEratosthenes(practicalLimit);
    if (practicalLimit < sqrtnp1) {
        knownPrimes.reserve(std::expint(log((double)sqrtnp1)) - std::expint(log(2)));
    }
    size_t count = knownPrimes.size();

    // Divide the range in different segments
    const size_t nCardinality = backward7(n);
    size_t low = backward7(practicalLimit);
    size_t high = low + limit;

    // Process one segment at a time till we pass n.
    while (low < nCardinality)
    {
        if (high > nCardinality) {
           high = nCardinality;
        }
        const BigInteger fLo = forward7(low);
        const size_t sqrtIndex = std::distance(
            knownPrimes.begin(),
            std::upper_bound(knownPrimes.begin(), knownPrimes.end(), sqrt(forward7(high)) + 1U)
        );

        const size_t cardinality = high - low;
        bool notPrime[cardinality + 1U] = { false };

        // Use the primes found by the simple sieve
        // to find primes in current range
        for (size_t k = 3U; k < sqrtIndex; ++k) {
            const BigInteger& p = knownPrimes[k];
            dispatch.dispatch([&n, &fLo, p, &notPrime]() {
                // We are skipping multiples of 2.
                const BigInteger p2 = p << 1U;

                // Find the minimum number in [low..high] that is
                // not a multiple of wheel factorization primes.
                BigInteger i = (fLo / p) * p;
                if (i < fLo) {
                    i += p;
                }
                if (!(i & 1U)) {
                    i += p;
                }
                if (!(i % 3U)) {
                    i += p2;
                }
                if (!(i % 5U)) {
                    i += p2;
                }
                if (!(i % 7U)) {
                    i += p2;
                }

                for (; i <= n; i += 11U * p) {
                    notPrime[backward7(i)] = true;
                }
                return false;
            });
        }
        dispatch.finish();

        if (knownPrimes.back() >= sqrtnp1) {
            for (size_t o = 1U; o <= cardinality; ++o) {
                if (!notPrime[o]) {
                    ++count;
                }
            }
        } else {
            for (size_t o = 1U; o <= cardinality; ++o) {
                if (!notPrime[o]) {
                    const BigInteger p = forward7(o + low);
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

std::string _SegmentedCountPrimesTo(const std::string& n) {
    return boost::lexical_cast<std::string>(SegmentedCountPrimesTo(BigInteger(n)));
}
} // namespace qimcifa

using namespace qimcifa;

PYBIND11_MODULE(_eratosthenes, m) {
    m.doc() = "pybind11 plugin to generate prime numbers";
    m.def("_count", &_CountPrimesTo, "Counts the prime numbers between 1 and the value of its argument");
    m.def("_segmented_count", &_SegmentedCountPrimesTo, "Counts the primes in capped space complexity");
    m.def("_sieve", &_SieveOfEratosthenes, "Returns all primes up to the value of its argument (using Sieve of Eratosthenes)");
    m.def("_segmented_sieve", &_SegmentedSieveOfEratosthenes, "Returns the primes in capped space complexity");
}
