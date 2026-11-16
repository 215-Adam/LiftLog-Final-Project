/* Paste into https://godbolt.org  (x86-64 gcc, flags: -O2)
 * Hot loop of optimized.c, self-contained. */
#include <stdint.h>
#include <stddef.h>

#define MAX_WEEKS 64

struct Set {
    float    weight;
    uint32_t user_id;
    uint32_t day;
    uint16_t exercise_id;
    uint16_t reps;
};

struct Stat {
    double best_e1rm;
    double pr_weight;
    double volume;
    int    sets;
    int    unused;
};

static double epley_e1rm(double weight, int reps)
{
    return weight * (1.0 + reps / 30.0);
}

static int week_of(uint32_t day)
{
    uint32_t w = day / 7;
    return (int)(w >= MAX_WEEKS ? MAX_WEEKS - 1 : w);
}

void compute_stats(const struct Set *sets, long n, struct Stat *stats,
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
