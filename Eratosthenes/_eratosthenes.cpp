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

DispatchQueue dispatch(std::thread::hardware_concurrency());

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

// We are multiplying out the first distinct primes, below.

// Make this NOT a multiple of 2.
inline BigInteger forward2(const size_t& p) { return (p << 1U) | 1U; }

inline size_t backward2(const BigInteger& p) { return (size_t)(p >> 1U); }

// Make this NOT a multiple of 2 or 3.
inline BigInteger forward3(const size_t& p) { return (p << 1U) + (~(~p | 1U)) - 1U; }

inline size_t backward3(const BigInteger& n) { return (size_t)((~(~n | 1U)) / 3U) + 1U; }

constexpr unsigned char wheel5[8U] = { 1U, 7U, 11U, 13U, 17U, 19U, 23U, 29U };

// Make this NOT a multiple of 2, 3, or 5.
BigInteger forward5(const size_t& p) { return wheel5[p % 8U] + (p / 8U) * 30U; }

size_t backward5(const BigInteger &n) {
    return std::distance(wheel5, std::lower_bound(wheel5, wheel5 + 8U, (size_t)(n % 30U))) + 8U * (size_t)(n / 30U) + 1U;
}

constexpr unsigned char wheel7[48U] = {
    1U, 11U, 13U, 17U, 19U, 23U, 29U, 31U, 37U, 41U, 43U, 47U, 53U, 59U, 61U, 67U, 71U, 73U, 79U, 83U, 89U,
    97U, 101U, 103U, 107U, 109U, 113U, 121U, 127U, 131U, 137U, 139U, 143U, 149U, 151U, 157U, 163U, 167U,
    169U, 173U, 179U, 181U, 187U, 191U, 193U, 197U, 199U, 209U
};

// Make this NOT a multiple of 2, 3, 5, or 7.
BigInteger forward7(const size_t& p) { return wheel7[p % 48U] + (p / 48U) * 210U; }

size_t backward7(const BigInteger& n) {
    return std::distance(wheel7, std::lower_bound(wheel7, wheel7 + 48U, (size_t)(n % 210U))) + 48U * (size_t)(n / 210U) + 1U;
}

constexpr unsigned short wheel11[480U] = {
    1U,    13U,   17U,   19U,   23U,   29U,   31U,   37U,   41U,   43U,   47U,   53U,   59U,   61U,   67U,   71U,   73U,   79U,   83U,   89U,   97U,   101U,  103U,  107U,
    109U,  113U,  127U,  131U,  137U,  139U,  149U,  151U,  157U,  163U,  167U,  169U,  173U,  179U,  181U,  191U,  193U,  197U,  199U,  211U,  221U,  223U,  227U,  229U,
    233U,  239U,  241U,  247U,  251U,  257U,  263U,  269U,  271U,  277U,  281U,  283U,  289U,  293U,  299U,  307U,  311U,  313U,  317U,  323U,  331U,  337U,  347U,  349U,
    353U,  359U,  361U,  367U,  373U,  377U,  379U,  383U,  389U,  391U,  397U,  401U,  403U,  409U,  419U,  421U,  431U,  433U,  437U,  439U,  443U,  449U,  457U,  461U,
    463U,  467U,  479U,  481U,  487U,  491U,  493U,  499U,  503U,  509U,  521U,  523U,  527U,  529U,  533U,  541U,  547U,  551U,  557U,  559U,  563U,  569U,  571U,  577U,
    587U,  589U,  593U,  599U,  601U,  607U,  611U,  613U,  617U,  619U,  629U,  631U,  641U,  643U,  647U,  653U,  659U,  661U,  667U,  673U,  677U,  683U,  689U,  691U,
    697U,  701U,  703U,  709U,  713U,  719U,  727U,  731U,  733U,  739U,  743U,  751U,  757U,  761U,  767U,  769U,  773U,  779U,  787U,  793U,  797U,  799U,  809U,  811U,
    817U,  821U,  823U,  827U,  829U,  839U,  841U,  851U,  853U,  857U,  859U,  863U,  871U,  877U,  881U,  883U,  887U,  893U,  899U,  901U,  907U,  911U,  919U,  923U,
    929U,  937U,  941U,  943U,  947U,  949U,  953U,  961U,  967U,  971U,  977U,  983U,  989U,  991U,  997U,  1003U, 1007U, 1009U, 1013U, 1019U, 1021U, 1027U, 1031U, 1033U,
    1037U, 1039U, 1049U, 1051U, 1061U, 1063U, 1069U, 1073U, 1079U, 1081U, 1087U, 1091U, 1093U, 1097U, 1103U, 1109U, 1117U, 1121U, 1123U, 1129U, 1139U, 1147U, 1151U, 1153U,
    1157U, 1159U, 1163U, 1171U, 1181U, 1187U, 1189U, 1193U, 1201U, 1207U, 1213U, 1217U, 1219U, 1223U, 1229U, 1231U, 1237U, 1241U, 1247U, 1249U, 1259U, 1261U, 1271U, 1273U,
    1277U, 1279U, 1283U, 1289U, 1291U, 1297U, 1301U, 1303U, 1307U, 1313U, 1319U, 1321U, 1327U, 1333U, 1339U, 1343U, 1349U, 1357U, 1361U, 1363U, 1367U, 1369U, 1373U, 1381U,
    1387U, 1391U, 1399U, 1403U, 1409U, 1411U, 1417U, 1423U, 1427U, 1429U, 1433U, 1439U, 1447U, 1451U, 1453U, 1457U, 1459U, 1469U, 1471U, 1481U, 1483U, 1487U, 1489U, 1493U,
    1499U, 1501U, 1511U, 1513U, 1517U, 1523U, 1531U, 1537U, 1541U, 1543U, 1549U, 1553U, 1559U, 1567U, 1571U, 1577U, 1579U, 1583U, 1591U, 1597U, 1601U, 1607U, 1609U, 1613U,
    1619U, 1621U, 1627U, 1633U, 1637U, 1643U, 1649U, 1651U, 1657U, 1663U, 1667U, 1669U, 1679U, 1681U, 1691U, 1693U, 1697U, 1699U, 1703U, 1709U, 1711U, 1717U, 1721U, 1723U,
    1733U, 1739U, 1741U, 1747U, 1751U, 1753U, 1759U, 1763U, 1769U, 1777U, 1781U, 1783U, 1787U, 1789U, 1801U, 1807U, 1811U, 1817U, 1819U, 1823U, 1829U, 1831U, 1843U, 1847U,
    1849U, 1853U, 1861U, 1867U, 1871U, 1873U, 1877U, 1879U, 1889U, 1891U, 1901U, 1907U, 1909U, 1913U, 1919U, 1921U, 1927U, 1931U, 1933U, 1937U, 1943U, 1949U, 1951U, 1957U,
    1961U, 1963U, 1973U, 1979U, 1987U, 1993U, 1997U, 1999U, 2003U, 2011U, 2017U, 2021U, 2027U, 2029U, 2033U, 2039U, 2041U, 2047U, 2053U, 2059U, 2063U, 2069U, 2071U, 2077U,
    2081U, 2083U, 2087U, 2089U, 2099U, 2111U, 2113U, 2117U, 2119U, 2129U, 2131U, 2137U, 2141U, 2143U, 2147U, 2153U, 2159U, 2161U, 2171U, 2173U, 2179U, 2183U, 2197U, 2201U,
    2203U, 2207U, 2209U, 2213U, 2221U, 2227U, 2231U, 2237U, 2239U, 2243U, 2249U, 2251U, 2257U, 2263U, 2267U, 2269U, 2273U, 2279U, 2281U, 2287U, 2291U, 2293U, 2297U, 2309U};

// Make this NOT a multiple of 2, 3, 5, 7, or 11.
BigInteger forward11(const size_t &p) { return wheel11[p % 480U] + (p / 480U) * 2310U; }

size_t backward11(const BigInteger &n) {
    return std::distance(wheel11, std::lower_bound(wheel11, wheel11 + 480U, (size_t)(n % 2310U))) + 480U * (size_t)(n / 2310U) + 1U;
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

    knownPrimes.reserve(std::expint(log((double)n)) - std::expint(log(2)));

    // We are excluding multiples of the first few
    // small primes from outset. For multiples of
    // 2, 3, and 5 this reduces complexity to 4/15.
    const size_t cardinality = backward5(n);

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
    unsigned short wheel5 = 129U;
    unsigned long long wheel7 = 9009416540524545ULL;
    size_t o = 1U;
    for (;;) {
        o += GetWheel5and7Increment(wheel5, wheel7);

        const BigInteger p = forward3(o);
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
        const BigInteger p = forward3(o);
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

std::vector<BigInteger> SegmentedSieveOfEratosthenes(BigInteger n)
{
    // TODO: This should scale to the system.
    // Assume the L1/L2 cache limit is 2048 KB.
    // We save half our necessary bytes by
    // removing multiples of 2.
    // The simple sieve removes multiples of 2, 3, and 5.
    // limit = 2048 KB = 2097152 B,
    // limit = ((((limit * 2) * 3) / 2) * 5) / 4
    constexpr size_t limit = 7864321ULL;

    if (!(n & 1U)) {
        --n;
    }
    while (((n % 3U) == 0) || ((n % 5U) == 0)) {
        n -= 2U;
    }
    if (limit >= n) {
        return SieveOfEratosthenes(n);
    }
    std::vector<BigInteger> knownPrimes = SieveOfEratosthenes(limit);
    knownPrimes.reserve(std::expint(log((double)n)) - std::expint(log(2)));

    // Divide the range in different segments
    const size_t nCardinality = backward5(n);
    size_t low = backward5(limit);
    size_t high = low + limit;

    // Process one segment at a time till we pass n.
    while (low < nCardinality)
    {
        if (high > nCardinality) {
           high = nCardinality;
        }

        const BigInteger fLo = forward5(low);
        const size_t sqrtIndex = std::distance(
            knownPrimes.begin(),
            std::upper_bound(knownPrimes.begin(), knownPrimes.end(), sqrt(forward5(high)) + 1U)
        );

        const size_t cardinality = high - low;
        bool notPrime[cardinality + 1U] = { false };

        for (size_t k = 3U; k < sqrtIndex; ++k) {
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
                    const size_t o = backward5(i) - low;
                    if (o > cardinality) {
                        return false;
                    }
                    if ((i % 3U) && (i % 5U)) {
                        notPrime[o] = true;
                    }
                    i += p2;
                }

                return false;
            });
        }
        dispatch.finish();

        // Numbers which are not marked are prime
        for (size_t o = 1U; o <= cardinality; ++o) {
            if (!notPrime[o]) {
                knownPrimes.push_back(forward5(o + low));
            }
        }

        // Update low and high for next segment
        low = low + limit;
        high = low + limit;
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
    const size_t cardinality = backward5(n);

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
    unsigned short wheel5 = 129U;
    unsigned long long wheel7 = 9009416540524545ULL;
    size_t o = 1U;
    BigInteger count = 4U;
    for (;;) {
        o += GetWheel5and7Increment(wheel5, wheel7);

        const BigInteger p = forward3(o);
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
        const BigInteger p = forward3(o);
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

BigInteger SegmentedCountPrimesTo(BigInteger n)
{
    // TODO: This should scale to the system.
    // Assume the L1/L2 cache limit is 2048 KB.
    // We save half our necessary bytes by
    // removing multiples of 2.
    // The simple sieve removes multiples of 2, 3, and 5.
    // limit = 2048 KB = 2097152 B,
    // limit = ((((limit * 2) * 3) / 2) * 5) / 4
    constexpr size_t limit = 7864321ULL;

    if (!(n & 1U)) {
        --n;
    }
    while (((n % 3U) == 0) || ((n % 5U) == 0)) {
        n -= 2U;
    }
    if (limit >= n) {
        return CountPrimesTo(n);
    }
    BigInteger sqrtnp1 = (sqrt(n) + 1U) | 1U;
    while (((sqrtnp1 % 3U) == 0U) || ((sqrtnp1 % 5U) == 0U)) {
        sqrtnp1 += 2U;
    }
    const BigInteger practicalLimit = (sqrtnp1 < limit) ? sqrtnp1 : limit;
    std::vector<BigInteger> knownPrimes = SieveOfEratosthenes(practicalLimit);
    if (practicalLimit < sqrtnp1) {
        knownPrimes.reserve(std::expint(log((double)sqrtnp1)) - std::expint(log(2)));
    }
    size_t count = knownPrimes.size();

    // Divide the range in different segments
    const size_t nCardinality = backward5(n);
    size_t low = backward5(practicalLimit);
    size_t high = low + limit;

    // Process one segment at a time till we pass n.
    while (low < nCardinality)
    {
        if (high > nCardinality) {
           high = nCardinality;
        }
        const BigInteger fLo = forward5(low);
        const size_t sqrtIndex = std::distance(
            knownPrimes.begin(),
            std::upper_bound(knownPrimes.begin(), knownPrimes.end(), sqrt(forward5(high)) + 1U)
        );

        const size_t cardinality = high - low;
        bool notPrime[cardinality + 1U] = { false };

        // Use the primes found by the simple sieve
        // to find primes in current range
        for (size_t k = 3U; k < sqrtIndex; ++k) {
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
                    const size_t o = backward5(i) - low;
                    if (o > cardinality) {
                        return false;
                    }
                    if ((i % 3U) && (i % 5U)) {
                        notPrime[o] = true;
                    }
                    i += p2;
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
                    const BigInteger p = forward5(o + low);
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

std::vector<std::string> _SieveOfEratosthenes(const std::string& n) {
    std::vector<BigInteger> v = SieveOfEratosthenes(BigInteger(n));
    std::vector<std::string> toRet;
    toRet.reserve(v.size());
    for (const BigInteger& p : v) {
        toRet.push_back(boost::lexical_cast<std::string>(p));
    }

    return toRet;
}

std::string _CountPrimesTo(const std::string& n) {
    return boost::lexical_cast<std::string>(CountPrimesTo(BigInteger(n)));
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
