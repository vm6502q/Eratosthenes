// Turn this off, if you're not factoring a semi-prime number with equal-bit-width factors.
#define IS_RSA_SEMIPRIME 1
// Turn this off, if you don't want to coordinate across multiple (quasi-independent) nodes.
#define IS_DISTRIBUTED 1
// Turn this off, if you don't want 'quantum-inspired' randomness.
/* #undef IS_RANDOM */
// Use GMP arbitrary precision integers, (or use Boost alternative, if turned off).
/* #undef USE_GMP */
// Use GMP arbitrary precision integers, (or use Boost alternative, if turned off).
#define USE_BOOST 1
// Optionally additionally check random number generator outputs for factoring via congruence of squares.
/* #undef IS_SQUARES_CONGRUENCE_CHECK */
// Bit width of (OpenCL) arbitrary precision "big integers"
#define BIG_INT_BITS 64
