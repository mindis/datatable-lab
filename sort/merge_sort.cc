//==============================================================================
// Micro benchmark for merge-sort functions
//==============================================================================
#include <string>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "sort.h"


static void iinsert_mergesort(int* x, int* o, int n, int i0)
{
  int i, j, xi, oi;
  for (i = i0; i < n; i++) {
    xi = x[i];
    if (xi < x[i-1]) {
      j = i - 1;
      oi = o[i];
      while (j >= 0 && xi < x[j]) {
        x[j+1] = x[j];
        o[j+1] = o[j];
        j--;
      }
      x[j+1] = xi;
      o[j+1] = oi;
    }
  }
}

static int compute_minrun(int n)
{
  int b = 0;
  // Testing manually, I find that MR=16 has a slight lead over MR=8, and
  // significantly better than MR=4, MR=32 or MR=64.
  while (n >= 16) {
    b |= n & 1;
    n >>= 1;
  }
  return n + b;
}



// Top-down mergesort
template <typename T>
void mergesort0_impl(T* x, int* o, int n, T* t, int* u, int P)
{
  if (n <= P) {
    insert_sort0<T>(x, o, n, 0);
    return;
  }
  // Sort each part recursively
  int n1 = n / 2;
  int n2 = n - n1;
  mergesort0_impl<T>(x, o, n1, t, u, P);
  mergesort0_impl<T>(x + n1, o + n1, n2, t + n1, u + n1, P);

  // Merge the parts
  std::memcpy(t, x, n1 * sizeof(T));
  std::memcpy(u, o, n1 * sizeof(int));
  int i = 0, j = 0, k = 0;
  T* x1 = t;
  T* x2 = x + n1;
  int* o1 = u;
  int* o2 = o + n1;
  while (1) {
    if (x1[i] <= x2[j]) {
      x[k] = x1[i];
      o[k] = o1[i];
      k++; i++;
      if (i == n1) {
        assert(j + n1 == k);
        break;
      }
    } else {
      x[k] = x2[j];
      o[k] = o2[j];
      k++; j++;
      if (j == n2) {
        memcpy(x + k, x1 + i, (n1 - i) * sizeof(int));
        memcpy(o + k, o1 + i, (n1 - i) * sizeof(int));
        break;
      }
    }
  }
}

// P - size below which the sort function falls back to insert sort
template <typename T, int P>
void merge_sort0(T* x, int* o, int n, int K) {
  assert(tmp1.size() >= n * sizeof(T));
  assert(tmp2.size() >= n * sizeof(int));
  mergesort0_impl<T>(x, o, n, tmp1.get<T>(), tmp2.get<int>(), P);
}

template void merge_sort0<uint8_t,  8>(uint8_t*,  int*, int, int);
template void merge_sort0<uint16_t, 8>(uint16_t*, int*, int, int);
template void merge_sort0<uint32_t, 8>(uint32_t*, int*, int, int);
template void merge_sort0<uint64_t, 8>(uint64_t*, int*, int, int);
template void merge_sort0<uint8_t,  12>(uint8_t*,  int*, int, int);
template void merge_sort0<uint16_t, 12>(uint16_t*, int*, int, int);
template void merge_sort0<uint32_t, 12>(uint32_t*, int*, int, int);
template void merge_sort0<uint64_t, 12>(uint64_t*, int*, int, int);
template void merge_sort0<uint8_t,  16>(uint8_t*,  int*, int, int);
template void merge_sort0<uint16_t, 16>(uint16_t*, int*, int, int);
template void merge_sort0<uint32_t, 16>(uint32_t*, int*, int, int);
template void merge_sort0<uint64_t, 16>(uint64_t*, int*, int, int);
template void merge_sort0<uint8_t,  20>(uint8_t*,  int*, int, int);
template void merge_sort0<uint16_t, 20>(uint16_t*, int*, int, int);
template void merge_sort0<uint32_t, 20>(uint32_t*, int*, int, int);
template void merge_sort0<uint64_t, 20>(uint64_t*, int*, int, int);
template void merge_sort0<uint8_t,  24>(uint8_t*,  int*, int, int);
template void merge_sort0<uint16_t, 24>(uint16_t*, int*, int, int);
template void merge_sort0<uint32_t, 24>(uint32_t*, int*, int, int);
template void merge_sort0<uint64_t, 24>(uint64_t*, int*, int, int);




//==============================================================================
// Bottom-up merge sort
//==============================================================================

void mergesort1(int* x, int* o, int n, int K)
{
  int* t = tmp1.get<int>();
  int* u = tmp2.get<int>();

  // printf("mergesort1(x=%p, o=%p, n=%d)\n", x, o, n);
  int minrun = compute_minrun(n);
  // printf("  minrun = %d\n", minrun);
  // printf("  x = ["); for(int i = 0; i < n; i++) printf("%d, ", x[i]); printf("\b\b]\n");

  // First, sort all minruns in-place
  // printf("  sorting all minruns...\n");
  for (int i = 0, nleft = n; nleft > 0; i += minrun, nleft -= minrun) {
    int nn = nleft >= minrun? minrun : nleft;
    iinsert_mergesort(x + i, o + i, nn, 1);
  }
  // printf("  x = ["); for(int i = 0; i < n; i++) printf("%d, ", x[i]); printf("\b\b]\n");

  // When flip is 0, the data is in `x` / `o`; if 1 then data is in `t` / `u`
  int flip = 0;
  int* ix = NULL, *ox = NULL, *io = NULL, *oo = NULL;
  for (int wA = minrun; wA < n; wA *= 2) {
    if (flip) {
      ix = t; ox = x;
      io = u; oo = o;
      flip = 0;
    } else {
      ix = x; ox = t;
      io = o; oo = u;
      flip = 1;
    }
    int wB = wA;
    // printf("  wA = %d\n", wA);
    for (int s = 0; s < n; s += 2*wA) {
      int* xA = ix + s;
      int* xB = ix + s + wA;
      int* oA = io + s;
      int* oB = io + s + wA;
      int* xR = ox + s;
      int* oR = oo + s;
      if (s + 2*wA > n) {
        if (s + wA >= n) {
          size_t sz = (n - s) * sizeof(int);
          memcpy(xR, xA, sz);
          memcpy(oR, oA, sz);
          break;
        }
        wB = n - (s + wA);
      }
      // printf("    s=%d..%d, wB=%d\n", s, s+wA+wB, wB);

      int i = 0, j = 0, k = 0;
      while (1) {
        if (xA[i] <= xB[j]) {
          xR[k] = xA[i];
          oR[k] = oA[i];
          k++; i++;
          if (i == wA) {
            size_t sz = (wB - j) * sizeof(int);
            memcpy(xR + k, xB + j, sz);
            memcpy(oR + k, oB + j, sz);
            break;
          }
        } else {
          xR[k] = xB[j];
          oR[k] = oB[j];
          k++; j++;
          if (j == wB) {
            size_t sz = (wA - i) * sizeof(int);
            memcpy(xR + k, xA + i, sz);
            memcpy(oR + k, oA + i, sz);
            break;
          }
        }
      }
    }
  }

  if (ox != x && ox && oo) {
    // printf("  flipping... x=%p, o=%p, ox=%p, oo=%p\n", x, o, ox, oo);
    memcpy(o, oo, n * sizeof(int));
    memcpy(x, ox, n * sizeof(int));
  }
  // printf("  res = ["); for(int i = 0; i < n; i++) printf("%d, ", x[i]); printf("\b\b]\n");
  // printf("  done.\n");
}



//==============================================================================
// TimSort
//==============================================================================

static int find_next_run_length(int* x, int* o, int n)
{
  if (n == 1) return 1;
  int xlast = x[1];
  int i = 2;
  if (x[0] <= xlast) {
    for (; i < n; i++) {
      int xi = x[i];
      if (xi < xlast) break;
      xlast = xi;
    }
  } else {
    for (; i < n; i++) {
      int xi = x[i];
      if (xi >= xlast) break;
      xlast = xi;
    }
    // Reverse direction of the run
    for (int j1 = 0, j2 = i - 1; j1 < j2; j1++, j2--) {
      int t = x[j1];
      x[j1] = x[j2];
      x[j2] = t;
      t = o[j1];
      o[j1] = o[j2];
      o[j2] = t;
    }
  }
  return i;
}


static void merge_chunks(int* x, int* o, int nA, int nB, int* t, int* u)
{
  memcpy(t, x, nA * sizeof(int));
  memcpy(u, o, nA * sizeof(int));
  int* xA = t;
  int* xB = x + nA;
  int* oA = u;
  int* oB = o + nA;

  int iA = 0, iB = 0, k = 0;
  while (1) {
    if (xA[iA] <= xB[iB]) {
      x[k] = xA[iA];
      o[k] = oA[iA];
      k++; iA++;
      if (iA == nA) {
        break;
      }
    } else {
      x[k] = xB[iB];
      o[k] = oB[iB];
      k++; iB++;
      if (iB == nB) {
        memcpy(x + k, xA + iA, (nA - iA) * sizeof(int));
        memcpy(o + k, oA + iA, (nA - iA) * sizeof(int));
        break;
      }
    }
  }
}


static void merge_stack(int* stack, int* stacklen, int* x, int* o, int* tmp1, int* tmp2)
{
  int sn = *stacklen;
  while (sn >= 3) {
    int iA = stack[sn-3];
    int iB = stack[sn-2];
    int iC = stack[sn-1];
    int iL = stack[sn];
    int nA = iB - iA;
    int nB = iC - iB;
    int nC = iL - iC;
    if (nA && nA <= nB + nC) {
      // Invariant |A| > |B| + |C| is violated
      if (nA < nC) {  // merge A and B
        merge_chunks(x + iA, o + iA, nA, nB, tmp1, tmp2);
        stack[sn-2] = iC;
        stack[sn-1] = iL;
      } else {  // merge B and C
        merge_chunks(x + iB, o + iB, nB, nC, tmp1, tmp2);
        stack[sn-1] = iL;
      }
    } else if (nB <= nC) {
      // Invariant |B| > |C| is violated: merge B and C
      merge_chunks(x + iB, o + iB, nB, nC, tmp1, tmp2);
      stack[sn-1] = iL;
    } else
      break;
    sn--;
  }
  *stacklen = sn;
}

static void final_merge_stack(int* stack, int* stacklen, int* x, int* o, int* tmp1, int* tmp2)
{
  int sn = *stacklen;
  while (sn >= 3) {
    int iB = stack[sn-2];
    int iC = stack[sn-1];
    int iL = stack[sn];
    merge_chunks(x + iB, o + iB, iC - iB, iL - iC, tmp1, tmp2);
    stack[sn-1] = iL;
    sn--;
  }
  *stacklen = sn;
}


void timsort(int* x, int* o, int n, int K)
{
  // printf("timsort(x=%p, o=%p, n=%d, tmp1=%p, tmp2=%p)\n", x, o, n, tmp1, tmp2);
  // printf("  x = ["); for(int i = 0; i < n; i++) printf("%d, ", x[i]); printf("\b\b]\n");
  int minrun = compute_minrun(n);
  // printf("  minrun = %d\n", minrun);
  int stack[85];
  stack[0] = 0;
  stack[1] = 0;
  int stacklen = 1;

  int i = 0;
  int nleft = n;
  while (nleft) {
    // printf("  [i=%d]\n", i);
    // Find the next ascending run; if it is too short then extend to
    // `min(minrun, nleft)` elements.
    int rl = find_next_run_length(x + i, o + i, nleft);
    // printf("    runL = %d\n", rl);
    if (rl < minrun) {
      int newrun = minrun <= nleft? minrun : nleft;
      iinsert_mergesort(x + i, o + i, newrun, rl);
      rl = newrun;
      // printf("    x = ["); for(int i = 0; i < n; i++) printf("%d, ", x[i]); printf("\b\b]\n");
      // printf("    runL = %d\n", rl);
    }
    // Push the run onto the stack, and then merge elements on the stack
    // if necessary
    stack[++stacklen] = i + rl;
    // printf("    stack = ["); for(int i = 0; i <= stacklen; i++) printf("%d, ", stack[i]); printf("\b\b]\n");
    // printf("    merging stack...\n");
    merge_stack(stack, &stacklen, x, o, tmp1.get<int>(), tmp2.get<int>());
    // printf("    x = ["); for(int i = 0; i < n; i++) printf("%d, ", x[i]); printf("\b\b]\n");
    // printf("    stack = ["); for(int i = 0; i <= stacklen; i++) printf("%d, ", stack[i]); printf("\b\b]\n");
    i += rl;
    nleft -= rl;
  }
  // printf("  prepare to do final merge of the stack...\n");
  // printf("    stack = ["); for(int i = 0; i <= stacklen; i++) printf("%d, ", stack[i]); printf("\b\b]\n");
  final_merge_stack(stack, &stacklen, x, o, tmp1.get<int>(), tmp2.get<int>());
  assert(stacklen == 2);
  // printf("  end\n");
}



//==============================================================================
// std::sort
//==============================================================================
#include <algorithm>

template <typename T>
void std_sort(xoitem<T>* xo, int n, int) {
  std::stable_sort(xo, xo + n,
                  [=](const xoitem<T>& a, const xoitem<T>& b) {
                    return a.x < b.x;
                  });
}

template void std_sort(xoitem<uint8_t>*, int, int);
template void std_sort(xoitem<uint16_t>*, int, int);
template void std_sort(xoitem<uint32_t>*, int, int);
template void std_sort(xoitem<uint64_t>*, int, int);
