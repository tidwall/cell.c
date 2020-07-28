#include "cell.h"


static const char HEX[] = "0123456789abcdef";

static const char TBL[] = ""
    "------------------------------------------------"
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09"
    "-------"
    "\x0A\x0B\x0C\x0D\x0E\x0F"
    "--------------------------"
    "\x0A\x0B\x0C\x0D\x0E\x0F"
    "---------------------------------------------------------"
    "---------------------------------------------------------"
    "---------------------------------------";


// The maximum coord that is less than 1.0. Equal to 1 - EPSILON.
#define MAX_COORD 0.99999999999999988897769753748434595763683319091796875

static double clip(double x) {
    return x < 0 ? 0 : x > MAX_COORD ? MAX_COORD : x;
}

// Bit interleaving thanks to the Daniel Lemire's blog entry:
// https://lemire.me/blog/2018/01/08/how-fast-can-you-bit-interleave-32-bit-integers/
static uint64_t interleave(uint32_t input) {
	uint64_t word = input;
	word = (word ^ (word << 16)) & 0x0000ffff0000ffff;
	word = (word ^ (word << 8)) & 0x00ff00ff00ff00ff;
	word = (word ^ (word << 4)) & 0x0f0f0f0f0f0f0f0f;
	word = (word ^ (word << 2)) & 0x3333333333333333;
	word = (word ^ (word << 1)) & 0x5555555555555555;
	return word;
}

static uint32_t deinterleave(uint64_t word) {
	word &= 0x5555555555555555;
	word = (word ^ (word >> 1)) & 0x3333333333333333;
	word = (word ^ (word >> 2)) & 0x0f0f0f0f0f0f0f0f;
	word = (word ^ (word >> 4)) & 0x00ff00ff00ff00ff;
	word = (word ^ (word >> 8)) & 0x0000ffff0000ffff;
	word = (word ^ (word >> 16)) & 0x00000000ffffffff;
	return (uint32_t)word;
}

// cell_xy_encode returns an encoded Cell from X/Y floating points.
// The input floating points must be within the range [0.0,1.0).
// Values outside that range are clipped.
cell_xy cell_xy_encode(double x, double y) {

	// Produce 32-bit integers for X/Y/Z/M -> A/B/C/D
	uint32_t a = (uint32_t)(clip(x) * (double)((int64_t)1 << 32));
	uint32_t b = (uint32_t)(clip(y) * (double)((int64_t)1 << 32));

	// Interleave A/B into 64-bit integers AB
	uint64_t ab = interleave(a)<<1 | interleave(b);

	return ab;
}

// cell_xyz_encode returns an encoded Cell from X/Y/Z floating points.
// The input floating points must be within the range [0.0,1.0).
// Values outside that range are clipped.
cell_xyz cell_xyz_encode(double x, double y, double z) {
    cell_xyzm xyzm = cell_xyzm_encode(x, y, z, 0);
    return (cell_xyz){ .hi = xyzm.hi, .lo = xyzm.lo };
}

// cell_xyzm_encode returns an encoded Cell from X/Y/Z/M floating points.
// The input floating points must be within the range [0.0,1.0).
// Values outside that range are clipped.
cell_xyzm cell_xyzm_encode(double x, double y, double z, double m) {

	// Produce 32-bit integers for X/Y/Z/M -> A/B/C/D
	uint32_t a = (uint32_t)(clip(x) * (double)((int64_t)1 << 32));
	uint32_t b = (uint32_t)(clip(y) * (double)((int64_t)1 << 32));
	uint32_t c = (uint32_t)(clip(z) * (double)((int64_t)1 << 32));
	uint32_t d = (uint32_t)(clip(m) * (double)((int64_t)1 << 32));

	// Interleave A/C and B/D into 64-bit integers AC and BD
    uint64_t ac = interleave(a)<<1 | interleave(c);
    uint64_t bd = interleave(b)<<1 | interleave(d);

	// Interleave AC/BD into a single 128-bit ABCD (hi/lo) integer
	uint64_t hi = interleave((uint32_t)(ac>>32))<<1 | interleave((uint32_t)(bd>>32));
	uint64_t lo = interleave((uint32_t)(ac))<<1 | interleave((uint32_t)(bd));

	return (cell_xyzm){ .hi = hi, .lo = lo };
}

// cell_xy_decode returns the decoded values from a cell.
void cell_xy_decode(cell_xy cell, double *x, double *y) {
	// Decoding is the inverse of the Encode logic.
	uint64_t ab = cell;
	uint32_t a = deinterleave(ab >> 1);
	uint32_t b = deinterleave(ab);
	*x = (double)a / (double)((int64_t)1 << 32);
	*y = (double)b / (double)((int64_t)1 << 32);
}

// cell_xyz_decode returns the decoded values from a cell.
void cell_xyz_decode(cell_xyz cell, double *x, double *y, double *z) {
    double m;
    cell_xyzm xyzm = (cell_xyzm){ .hi = cell.hi, .lo = cell.lo };
    cell_xyzm_decode(xyzm, x, y, z, &m);
}

// cell_xyzm_decode returns the decoded values from a cell.
void cell_xyzm_decode(cell_xyzm cell, double *x, double *y, double *z, double *m) {
	// Decoding is the inverse of the Encode logic.
	uint64_t ac = ((uint64_t)(deinterleave(cell.hi>>1)) << 32) | (uint64_t)(deinterleave(cell.lo>>1));
	uint64_t bd = ((uint64_t)(deinterleave(cell.hi)) << 32) | (uint64_t)(deinterleave(cell.lo));
	uint32_t a = deinterleave(ac >> 1);
	uint32_t b = deinterleave(bd >> 1);
	uint32_t c = deinterleave(ac);
	uint32_t d = deinterleave(bd);
	*x = (double)a / (double)((uint64_t)1 << 32);
	*y = (double)b / (double)((uint64_t)1 << 32);
	*z = (double)c / (double)((uint64_t)1 << 32);
	*m = (double)d / (double)((uint64_t)1 << 32);
}

// cell_xy_compare compares two cells.
int cell_xy_compare(cell_xy a, cell_xy b) {
    return a < b ? -1 : a > b ? 1 : 0;
}

// cell_xyz_compare compares two cells.
int cell_xyz_compare(cell_xyz a, cell_xyz b) {
    return a.hi < b.hi ? -1 : a.hi > b.hi ? 1 : 
           a.lo < b.lo ? -1 : a.lo > b.lo ? 1 : 0;
}

// cell_xyzm_compare compares two cells.
int cell_xyzm_compare(cell_xyzm a, cell_xyzm b) {
    return a.hi < b.hi ? -1 : a.hi > b.hi ? 1 : 
           a.lo < b.lo ? -1 : a.lo > b.lo ? 1 : 0;
}

// cell_xy_string returns a string representation of the cell. 
// The input string buffer must be at least 17 bytes.
const char *cell_xy_string(cell_xy cell, char *str) {
    int j = 0;
	for (int i = 0; i < 64; i += 4) {
        str[j++] = HEX[(cell>>(60-i))&15];
	}
    str[j] = '\0';
	return str;
}

// cell_xyz_string returns a string representation of the cell. 
// The input string buffer must be at least 17 bytes.
const char *cell_xyz_string(cell_xyz cell, char *str) {
    cell_xyzm xyzm = (cell_xyzm){ .hi = cell.hi, .lo = cell.lo };
    return cell_xyzm_string(xyzm, str);
}


// cell_xyzm_string returns a string representation of the cell. 
// The input string buffer must be at least 33 bytes.
const char *cell_xyzm_string(cell_xyzm cell, char *str) {
    int j = 0;
	for (int i = 0; i < 64; i += 4) {
        str[j++] = HEX[(cell.hi>>(60-i))&15];
	}
	for (int i = 0; i < 64; i += 4) {
        str[j++] = HEX[(cell.lo>>(60-i))&15];
	}
    str[j] = '\0';
	return str;
}

// cell_xy returns a cell from a string representation.
cell_xy cell_xy_from_string(const char *str) {
    cell_xy cell = { 0 };
    if (str) {
        for (int i = 0; str[i] && i < 16; i++) {
        	cell = (cell << 4) | (uint64_t)(TBL[(int)str[i]]);
        }
    }
    return cell;
}

// cell_xyz returns a cell from a string representation.
cell_xyz cell_xyz_from_string(const char *str) {
    cell_xyzm xyzm = cell_xyzm_from_string(str);
    return (cell_xyz){ .lo = xyzm.lo, .hi = xyzm.hi };
}


// cell_xyzm returns a cell from a string representation.
cell_xyzm cell_xyzm_from_string(const char *str) {
    cell_xyzm cell = { 0 };
    if (str) {
        for (int i = 0; str[i] && i < 16; i++) {
        	cell.hi = (cell.hi << 4) | (uint64_t)(TBL[(int)str[i]]);
        }
        for (int i = 16; str[i] && i < 32; i++) {
        	cell.lo = (cell.lo << 4) | (uint64_t)(TBL[(int)str[i]]);
        }
    }
    return cell;
}

//==============================================================================
// TESTS AND BENCHMARKS
// $ cc -DCELL_TEST cell.c && ./a.out              # run tests
// $ cc -DCELL_TEST -O3 cell.c && BENCH=1 ./a.out  # run benchmarks
//==============================================================================
#ifdef CELL_TEST

#pragma GCC diagnostic ignored "-Wextra"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>

bool pretty_close(double x, double y) {
	return fabs(x-y) < 0.00001;
}

void test_xy(int N) {
    // run for 100 milliseconds
	int count = 0;
    char str[128];
    for (int i = 0; i < N; i++) {
        double x1 = (double)rand()/(double)INT_MAX;
        double y1 = (double)rand()/(double)INT_MAX;
        switch (i % 1000) {
        case 543: x1 = -0.0000001; break;
        case 264: y1 = -0.0000001; break;
        case 643: x1 = 1.0000001; break;
        case 129: y1 = 1.0000001; break;
        }
        cell_xy cell = cell_xy_encode(x1, y1);
        double x2, y2;
        cell_xy_decode(cell, &x2, &y2);
        assert(pretty_close(x1, x2) && pretty_close(y1, y2));
        cell_xy_string(cell, str);
        cell_xy cell2 = cell_xy_from_string(str);
        assert(cell_xy_compare(cell, cell2) == 0);
        count++;
    }
}
void test_xyz(int N) {
    // run for 100 milliseconds
	int count = 0;
    char str[128];
    for (int i = 0; i < N; i++) {
        double x1 = (double)rand()/(double)INT_MAX;
        double y1 = (double)rand()/(double)INT_MAX;
        double z1 = (double)rand()/(double)INT_MAX;
        switch (i % 1000) {
        case 543: x1 = -0.0000001; break;
        case 264: y1 = -0.0000001; break;
        case 812: z1 = -0.0000001; break;
        case 643: x1 = 1.0000001; break;
        case 129: y1 = 1.0000001; break;
        case 362: z1 = 1.0000001; break;
        }
        cell_xyz cell = cell_xyz_encode(x1, y1, z1);
        double x2, y2, z2;
        cell_xyz_decode(cell, &x2, &y2, &z2);
        assert(pretty_close(x1, x2) && pretty_close(y1, y2) && pretty_close(z1, z2));
        cell_xyz_string(cell, str);
        cell_xyz cell2 = cell_xyz_from_string(str);
        assert(cell_xyz_compare(cell, cell2) == 0);
        count++;
    }
}
void test_xyzm(int N) {
    // run for 250 milliseconds
	int count = 0;
    char str[128];
    for (int i = 0; i < N; i++) {
        double x1 = (double)rand()/(double)INT_MAX;
        double y1 = (double)rand()/(double)INT_MAX;
        double z1 = (double)rand()/(double)INT_MAX;
        double m1 = (double)rand()/(double)INT_MAX;
        switch (i % 1000) {
        case 543: x1 = -0.0000001; break;
        case 264: y1 = -0.0000001; break;
        case 812: z1 = -0.0000001; break;
        case 912: m1 = -0.0000001; break;
        case 643: x1 = 1.0000001; break;
        case 129: y1 = 1.0000001; break;
        case 362: z1 = 1.0000001; break;
        case 429: m1 = 1.0000001; break;
        }
        cell_xyzm cell = cell_xyzm_encode(x1, y1, z1, m1);
        double x2, y2, z2, m2;
        cell_xyzm_decode(cell, &x2, &y2, &z2, &m2);
        assert(pretty_close(x1, x2) && pretty_close(y1, y2) && pretty_close(z1, z2) && pretty_close(m1, m2));
        cell_xyzm_string(cell, str);
        cell_xyzm cell2 = cell_xyzm_from_string(str);
        assert(cell_xyzm_compare(cell, cell2) == 0);
        count++;
    }
}

size_t total_allocs = 0;
size_t total_mem = 0;

#define bench(name, N, code) {{ \
    if (strlen(name) > 0) { \
        printf("%-14s ", name); \
    } \
    size_t tmem = total_mem; \
    size_t tallocs = total_allocs; \
    uint64_t bytes = 0; \
    clock_t begin = clock(); \
    for (int i = 0; i < N; i++) { \
        (code); \
    } \
    clock_t end = clock(); \
    double elapsed_secs = (double)(end - begin) / CLOCKS_PER_SEC; \
    double bytes_sec = (double)bytes/elapsed_secs; \
    printf("%d ops in %.3f secs, %.0f ns/op, %.0f op/sec", \
        N, elapsed_secs, \
        elapsed_secs/(double)N*1e9, \
        (double)N/elapsed_secs \
    ); \
    if (bytes > 0) { \
        printf(", %.1f GB/sec", bytes_sec/1024/1024/1024); \
    } \
    if (total_mem > tmem) { \
        size_t used_mem = total_mem-tmem; \
        printf(", %.2f bytes/op", (double)used_mem/N); \
    } \
    if (total_allocs > tallocs) { \
        size_t used_allocs = total_allocs-tallocs; \
        printf(", %.2f allocs/op", (double)used_allocs/N); \
    } \
    printf("\n"); \
}}

struct point_xy { double x, y; };

void bench_xy(int N) {
    struct point_xy *points = malloc(sizeof(struct point_xy)*N);
    cell_xy *cells = malloc(sizeof(cell_xy)*N);
	for (int i = 0; i < N; i++) {
        points[i].x = (double)rand()/(double)INT_MAX;
        points[i].y = (double)rand()/(double)INT_MAX;
        cells[i] = cell_xy_encode(points[i].x, points[i].y);
	}
    double res = 0;
	bench("xy encode", N, {
        cell_xy cell = cell_xy_encode(points[i].x, points[i].y);
        res += cell;
    });
    double x, y;
	bench("xy decode", N, {
        cell_xy_decode(cells[i], &x, &y);
        res += x;
    });
    assert(res != 0);
}

struct point_xyz { double x, y, z; };

void bench_xyz(int N) {
    struct point_xyz *points = malloc(sizeof(struct point_xyz)*N);
    cell_xyz *cells = malloc(sizeof(cell_xyz)*N);
	for (int i = 0; i < N; i++) {
        points[i].x = (double)rand()/(double)INT_MAX;
        points[i].y = (double)rand()/(double)INT_MAX;
        points[i].z = (double)rand()/(double)INT_MAX;
        cells[i] = cell_xyz_encode(points[i].x, points[i].y, points[i].z);
	}
    double res = 0;
	bench("xyz encode", N, {
        cell_xyz cell = cell_xyz_encode(points[i].x, points[i].y, points[i].z);
        res += cell.lo;
    });
    double x, y, z;
	bench("xyz decode", N, {
        cell_xyz_decode(cells[i], &x, &y, &z);
        res += x;
    });
    assert(res != 0);
}

struct point_xyzm { double x, y, z, m; };

void bench_xyzm(int N) {
    struct point_xyzm *points = malloc(sizeof(struct point_xyzm)*N);
    cell_xyzm *cells = malloc(sizeof(cell_xyzm)*N);
	for (int i = 0; i < N; i++) {
        points[i].x = (double)rand()/(double)INT_MAX;
        points[i].y = (double)rand()/(double)INT_MAX;
        points[i].z = (double)rand()/(double)INT_MAX;
        points[i].m = (double)rand()/(double)INT_MAX;
        cells[i] = cell_xyzm_encode(points[i].x, points[i].y, points[i].z, points[i].m);
	}
    double res = 0;
	bench("xyzm encode", N, {
        cell_xyzm cell = cell_xyzm_encode(points[i].x, points[i].y, points[i].z, points[i].m);
        res += cell.lo;
    });
    double x, y, z, m;
	bench("xyzm decode", N, {
        cell_xyzm_decode(cells[i], &x, &y, &z, &m);
        res += x;
    });
    assert(res != 0);
}


int main() {
    int seed = getenv("SEED")?atoi(getenv("SEED")):time(NULL);
    int N = getenv("N")?atoi(getenv("N")):1000000;
    printf("seed=%d, count=%d\n", seed, N);
    srand(seed);
    if (getenv("BENCH")) {
        printf("Running cell benchmarks...\n");
        bench_xy(N);
        bench_xyz(N);
        bench_xyzm(N);
    } else {
        printf("Running cell tests...\n");
        test_xy(N);
        test_xyz(N);
        test_xyzm(N);
        printf("PASSED\n");
    }
}

#endif
