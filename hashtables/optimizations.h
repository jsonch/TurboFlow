#include "flat_hash_map.hpp"
#include "xxhash.hpp"

/*==============================================
=            128 bit key comparison            =
==============================================*/
namespace std
{
    template <>
    struct hash<__m128i>
    {
        size_t operator()(const __m128i& k) const
        {
            return XXHash32::hash((void *)&k, 16, 1);
        }
    };
}

// https://stackoverflow.com/questions/44511386/sse-addition-and-conversion
static const __m128i zero = {0};
inline bool compare128(__m128i a, __m128i b) {
    __m128i c = _mm_xor_si128(a, b);
    return _mm_testc_si128(zero, c);
}
 
typedef struct
{
  bool operator() (const __m128i &x, const __m128i &y) const { 
    // __m128i* a = (__m128i*) &x;
    // __m128i* b = (__m128i*) &y;

    return compare128 (x, y); 
  }
} AggregateKeyEq;
 
ska::flat_hash_map<__m128i, int, std::hash<__m128i>, AggregateKeyEq> flow_map;


/*=====  End of 128 bit key comparison  ======*/

#ifdef BATCHING
/*====================================================
=            Loop unrolling and batching.            =
====================================================*/
#define PREFETCHCT 10



// Loop unrolling.
template <int N> struct _int{ };

template <int N, typename F, typename ...Args>
inline void unroll_f(_int<N>, F&& f, Args&&... args) {      
    unroll_f(_int<N-1>(),std::forward<F>(f),std::forward<Args>(args)...);
    f(N,args...);
}
template <typename F, typename ...Args>
inline void unroll_f(_int<0>, F&& f, Args&&... args) {
    f(0,args...);
}
__m128i * v[PREFETCHCT];
void * ptrs[PREFETCHCT];

/*=====  End of Loop unrolling and batching.  ======*/

#endif



/*=======================================
=            Cycle counting.            =
=======================================*/

typedef unsigned long long ticks;

static __inline__ ticks tickStart (void) {
  unsigned cycles_low, cycles_high;
  asm volatile ("CPUID\n\t"
        "RDTSC\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low)::
        "%rax", "%rbx", "%rcx", "%rdx");
  return ((ticks)cycles_high << 32) | cycles_low;
}

static __inline__ ticks tickStop (void) {
  unsigned cycles_low, cycles_high;
  asm volatile("RDTSCP\n\t"
           "mov %%edx, %0\n\t"
           "mov %%eax, %1\n\t"
           "CPUID\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax",
           "%rbx", "%rcx", "%rdx");
  return ((ticks)cycles_high << 32) | cycles_low;
}

/*=====  End of Cycle counting.  ======*/
