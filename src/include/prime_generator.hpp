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

#include "config.h"

#include <vector>

#if BIG_INT_BITS > 64
#include <boost/multiprecision/cpp_int.hpp>
#endif

#include "dispatchqueue.hpp"

namespace qimcifa {
DispatchQueue dispatch(std::thread::hardware_concurrency());

#if BIG_INT_BITS < 33
typedef uint32_t BigInteger;
#elif BIG_INT_BITS < 65
typedef uint64_t BigInteger;
#else
typedef boost::multiprecision::number<boost::multiprecision::cpp_int_backend<BIG_INT_BITS, BIG_INT_BITS,
    boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>
    BigInteger;
#endif

inline BigInteger backward(const BigInteger& n) {
    return ((~((~n) | 1U)) / 3U) + 1U;
}

inline BigInteger forward(const BigInteger& p) {
    // Make this NOT a multiple of 2 or 3.
    return (p << 1U) + (~(~p | 1U)) - 1U;
}

inline BigInteger backward5(BigInteger n) {
    n = ((n + 1U) << 2U) / 5U;
    n = ((n + 1U) << 1U) / 3U;
    return (n + 1U) >> 1U;
}

inline size_t GetWheel5Increment(uint32_t& wheel5) {
    size_t wheelIncrement = 0U;
    bool is_wheel_multiple = false;
    do {
        is_wheel_multiple = (bool)(wheel5 & 1U);
        wheel5 >>= 1U;
        if (is_wheel_multiple) {
            wheel5 |= 1U << 9U;
        }
        wheelIncrement++;
    } while (is_wheel_multiple);

    return wheelIncrement;
}

std::vector<BigInteger> SieveOfEratosthenes(const BigInteger& n);
BigInteger CountPrimesTo(const BigInteger& n);
} // namespace qimcifa
