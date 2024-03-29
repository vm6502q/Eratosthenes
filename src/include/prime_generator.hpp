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
    return (p << 1U) - 1U;
}

inline BigInteger forward(const size_t& p) {
    // Make this NOT a multiple of 2 or 3.
    return (p << 1U) + (~(~p | 1U)) - 1U;
}

inline size_t backward2(const BigInteger& p) {
    return (p + 1U) >> 1U;
}

inline size_t backward(const BigInteger& n) {
    return ((~((~n) | 1U)) / 3U) + 1U;
}

inline size_t backward5(const BigInteger& n) {
    return (((((n + 1U) << 2U) / 5U + 1U) << 1U) / 3U + 1U) >> 1U;
}

inline size_t backward7(const BigInteger& n) {
    constexpr unsigned char m[48U] = {
        1U, 11U, 13U, 17U, 19U, 23U, 29U, 31U, 37U, 41U, 43U, 47U, 53U, 59U, 61U, 67U, 71U, 73U, 79U, 83U, 89U,
        97U, 101U, 103U, 107U, 109U, 113U, 121U, 127U, 131U, 137U, 139U, 143U, 149U, 151U, 157U, 163U, 167U,
        169U, 173U, 179U, 181U, 187U, 191U, 193U, 197U, 199U, 209U
    };
    return std::distance(m, std::lower_bound(m, m + 48U, n % 210U)) + 48U * (n / 210U) + 1U;
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

inline BigInteger makeNotSpaceMultiple(BigInteger n) {
    if ((n & 1U) == 0U) {
        --n;
    }
    while (((n % 3U) == 0U) || ((n % 5U) == 0U)) {
        n -= 2U;
    }

    return n;
}

BigInteger CountPrimesTo(const BigInteger& n);
std::vector<BigInteger> SieveOfEratosthenes(const BigInteger& n);
std::vector<BigInteger> SegmentedSieveOfEratosthenes(BigInteger n, size_t limit);
} // namespace qimcifa
