/*
 * optimized.c - LiftLog, MVP optimized version.
 *
 * Same input, same report, same timing instrumentation as baseline.c.
 *
 * PRIMARY OPTIMIZATION (MVP): replace per-record heap allocation and
 * pointer-chasing data structures with a few large, contiguous allocations.
 *   - The whole log is read into ONE buffer with a single fread.
 *   - All sets live in ONE array of 16-byte `struct Set`, allocated once at
 *     the exact size (lines are counted first), fields ordered largest to
 *     smallest so there is no padding and no pointers.
 *   - Exercise names are interned once into a small arena; each set stores a
 *     2-byte exercise_id instead of a pointer to its own strdup'd string.
 *   - Stats are a flat [user][exercise] table, so the hot loop does one
 *     index calculation instead of walking a linked list with strcmp.
 *   - Cleanup is a handful of free() calls instead of ~2 per set.
 *
 * Deliberately NOT changed yet (planned for the final submission): the
 * field parsing still uses strtok/atoi/atof/sscanf exactly like the baseline,
 * sets are not sorted by user, and helpers are not hand-tuned for inlining.
 *
 * Usage:   optimized LOG.csv > report.txt
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L /* clock_gettime */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "liftlog_common.h"

/* ---- allocation counting (same instrumentation as baseline) ---- */
static long malloc_calls = 0;
static long free_calls = 0;

static void *counted_malloc(size_t n)
{
    void *p = malloc(n);
    if (!p) { perror("malloc"); exit(1); }
    malloc_calls++;
    return p;
}

static void *counted_calloc(size_t count, size_t n)
{
    void *p = calloc(count, n);
    if (!p) { perror("calloc"); exit(1); }
    malloc_calls++;
    return p;
}

static void counted_free(void *p)
{
    if (p) { free(p); free_calls++; }
}

/* ---- data structures ---- */
struct Set {                 /* 16 bytes, no padding, no pointers */
    float    weight;
    uint32_t user_id;
    uint32_t day;            /* days since 2024-01-01 */
    uint16_t exercise_id;
    uint16_t reps;
};
_Static_assert(sizeof(struct Set) == 16, "struct Set must be 16 bytes");

struct Stat {                /* one cell of the [user][exercise] table */
    double best_e1rm;
    double pr_weight;
    double volume;
    int    sets;
    int    unused;           /* explicit, keeps the struct a clean 32 bytes */
};

/* ---- exercise-name interning (arena + small open-addressing hash) ---- */
#define MAX_EXERCISES 256
#define NAME_ARENA_BYTES 8192
#define HASH_SLOTS 512       /* power of two, > 2 * MAX_EXERCISES */

struct Interner {
    char *arena;             /* every name stored back to back, NUL-terminated */
    size_t arena_used;
    const char *names[MAX_EXERCISES];
    int count;
    int16_t slots[HASH_SLOTS]; /* -1 = empty, else exercise id */
};

static uint32_t fnv1a(const char *s)
{
    uint32_t h = 2166136261u;
    while (*s) { h ^= (unsigned char)*s++; h *= 16777619u; }
    return h;
}

/* Returns the id for `name`, adding it the first time it is seen. */
static int intern(struct Interner *in, const char *name)
{
    uint32_t i = fnv1a(name) & (HASH_SLOTS - 1);
    while (in->slots[i] >= 0) {
        if (strcmp(in->names[in->slots[i]], name) == 0)
            return in->slots[i];
        i = (i + 1) & (HASH_SLOTS - 1);
    }
    size_t len = strlen(name) + 1;
    if (in->count >= MAX_EXERCISES || in->arena_used + len > NAME_ARENA_BYTES) {
        fprintf(stderr, "too many distinct exercise names\n");
        exit(1);
    }
    char *dst = in->arena + in->arena_used;
    memcpy(dst, name, len);
    in->arena_used += len;
    in->names[in->count] = dst;
    in->slots[i] = (int16_t)in->count;
    return in->count++;
}

/* ---- loading ---- */
static long parse_date(const char *s)
{
    int y = 0, m = 0, d = 0;
    if (sscanf(s, "%d-%d-%d", &y, &m, &d) != 3) return -1;
    return days_from_civil(y, m, d);
}

/* Reads the entire file with one fread. Returns a NUL-terminated buffer. */
static char *read_file(const char *path, size_t *size_out)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    if (fseek(f, 0, SEEK_END) != 0) { perror("fseek"); exit(1); }
    long size = ftell(f);
    if (size < 0) { perror("ftell"); exit(1); }
    rewind(f);
    char *buf = counted_malloc((size_t)size + 1);
    size_t got = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[got] = '\0';
    *size_out = got;
    return buf;
}

/* Parses every line of `buf` into one pre-sized array of struct Set. */
static struct Set *parse_sets(char *buf, size_t size, struct Interner *in,
                              long start, long *count, uint32_t *max_user)
{
    /* Pass 1: count lines so the array is allocated exactly once. */
    size_t lines = 1;
    for (char *p = buf; (p = memchr(p, '\n', size - (size_t)(p - buf))) != NULL; p++)
        lines++;
    struct Set *sets = counted_malloc(lines * sizeof *sets);

    /* Pass 2: split lines in place, parse fields (same calls as baseline). */
    long n = 0;
    *max_user = 0;
    char *line = buf;
    char *end = buf + size;
    while (line < end) {
        char *nl = memchr(line, '\n', (size_t)(end - line));
        char *next = nl ? nl + 1 : end;
        if (nl) *nl = '\0';

        char *tok_user = strtok(line, ",");
        char *tok_date = strtok(NULL, ",");
        char *tok_name = strtok(NULL, ",");
        char *tok_weight = strtok(NULL, ",");
        char *tok_reps = strtok(NULL, ",\r\n");
        line = next;
        if (!tok_user || !tok_date || !tok_name || !tok_weight || !tok_reps)
            continue; /* skip blank / malformed lines, like the baseline */

        int uid = atoi(tok_user);
        if (uid < 0) continue;
        long day = parse_date(tok_date) - start;

        struct Set *s = &sets[n++];
        s->user_id = (uint32_t)uid;
        s->day = (uint32_t)(day < 0 ? 0 : day); /* week 0, same as baseline clamp */
        s->exercise_id = (uint16_t)intern(in, tok_name);
        s->weight = (float)atof(tok_weight);
        s->reps = (uint16_t)atoi(tok_reps);
        if (s->user_id > *max_user) *max_user = s->user_id;
    }
    *count = n;
    return sets;
}

/* ---- stats ---- */
static double epley_e1rm(double weight, int reps)
{
    return weight * (1.0 + reps / 30.0);
}

static int week_of(uint32_t day)
{
    uint32_t w = day / 7;
    return (int)(w >= MAX_WEEKS ? MAX_WEEKS - 1 : w);
}

/* The hot loop: sequential walk over a contiguous array, one table lookup. */
static void compute_stats(const struct Set *sets, long n, struct Stat *stats,
                          double *weekly, int num_ex)
{
    for (long i = 0; i < n; i++) {
        const struct Set *s = &sets[i];
        struct Stat *stat = &stats[(size_t)s->user_id * num_ex + s->exercise_id];

        double w = s->weight;
        double e1rm = epley_e1rm(w, s->reps);
        double vol = w * s->reps;
        if (e1rm > stat->best_e1rm) stat->best_e1rm = e1rm;
        if (w > stat->pr_weight) stat->pr_weight = w;
        stat->volume += vol;
        stat->sets++;
        weekly[(size_t)s->user_id * MAX_WEEKS + week_of(s->day)] += vol;
    }
}

/* ---- report (identical format to baseline) ---- */
static const struct Interner *sort_ctx;

static int cmp_ex_id(const void *a, const void *b)
{
    int ia = *(const int *)a, ib = *(const int *)b;
    return strcmp(sort_ctx->names[ia], sort_ctx->names[ib]);
}

static void print_summary(FILE *out, const struct Stat *stats, const double *weekly,
                          uint32_t max_user, const struct Interner *in)
{
    int num_ex = in->count;
    /* Sort exercise ids by name once, instead of once per user. */
    int *order = counted_malloc((size_t)(num_ex > 0 ? num_ex : 1) * sizeof *order);
    for (int e = 0; e < num_ex; e++) order[e] = e;
    sort_ctx = in;
    qsort(order, (size_t)num_ex, sizeof *order, cmp_ex_id);

    for (uint32_t u = 0; u <= max_user; u++) {
        const struct Stat *row = &stats[(size_t)u * num_ex];
        int any = 0;
        for (int e = 0; e < num_ex; e++) if (row[e].sets) { any = 1; break; }
        if (!any) continue;

        const double *wk = &weekly[(size_t)u * MAX_WEEKS];
        double total = 0.0, best = -1.0;
        int best_week = 0;
        for (int w = 0; w < MAX_WEEKS; w++) {
            total += wk[w];
            if (wk[w] > best) { best = wk[w]; best_week = w; }
        }
        fprintf(out, "user %u: total_volume=%.1f best_week=%d week_volume=%.1f\n",
                (unsigned)u, total, best_week + 1, best);

        for (int k = 0; k < num_ex; k++) {
            const struct Stat *st = &row[order[k]];
            if (!st->sets) continue;
            fprintf(out, "  %s: e1rm=%.2f pr=%.1f volume=%.1f sets=%d\n",
                    in->names[order[k]], st->best_e1rm, st->pr_weight,
                    st->volume, st->sets);
        }
    }
    counted_free(order);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s LOG.csv > report.txt\n", argv[0]);
        return 1;
    }
    long start = days_from_civil(LOG_START_YEAR, 1, 1);

    struct Interner in;
    memset(&in, 0, sizeof in);
    memset(in.slots, 0xff, sizeof in.slots); /* all slots = -1 */

    long count;
    uint32_t max_user;
    size_t size;

    double t0 = now_ms();
    in.arena = counted_malloc(NAME_ARENA_BYTES);
    char *buf = read_file(argv[1], &size);
    struct Set *sets = parse_sets(buf, size, &in, start, &count, &max_user);
    double t1 = now_ms();

    size_t rows = (size_t)max_user + 1;
    int num_ex = in.count > 0 ? in.count : 1;
    struct Stat *stats = counted_calloc(rows * (size_t)num_ex, sizeof *stats);
    double *weekly = counted_calloc(rows * MAX_WEEKS, sizeof *weekly);
    double t2 = now_ms();
    compute_stats(sets, count, stats, weekly, num_ex);
    double t3 = now_ms();

    print_summary(stdout, stats, weekly, max_user, &in);
    fflush(stdout);

    double t4 = now_ms();
    counted_free(weekly);
    counted_free(stats);
    counted_free(sets);
    counted_free(buf);
    counted_free(in.arena);
    double t5 = now_ms();

    double load_ms = t1 - t0, process_ms = t3 - t2, cleanup_ms = t5 - t4;
    fprintf(stderr,
            "optimized: sets=%ld load_ms=%.2f process_ms=%.2f cleanup_ms=%.2f "
            "total_ms=%.2f mallocs=%ld frees=%ld\n",
            count, load_ms, process_ms, cleanup_ms,
            load_ms + process_ms + cleanup_ms, malloc_calls, free_calls);
    return 0;
}
