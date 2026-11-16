/*
 * gen_log.c - generates a synthetic LiftLog workout log.
 *
 * Usage:  gen_log NUM_SETS NUM_USERS OUTPUT.csv [SEED]
 *
 * Each line is one set:  user_id,YYYY-MM-DD,exercise,weight,reps
 *   e.g.  4821,2024-03-17,Bench Press,185.0,5
 *
 * Lines are in date order across one year starting 2024-01-01 (like a real
 * app export). Weights are always multiples of 2.5 lb (real plate math), and
 * the output is fully deterministic for a given seed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "liftlog_common.h"

static const char *EXERCISES[] = {
    "Back Squat",     "Front Squat",   "Bench Press",    "Incline Bench Press",
    "Deadlift",       "Romanian Deadlift", "Overhead Press", "Barbell Row",
    "Pull Up",        "Chin Up",       "Dip",            "Lat Pulldown",
    "Leg Press",      "Hip Thrust",    "Lunge",          "Barbell Curl",
    "Skull Crusher",  "Seated Cable Row", "Calf Raise",  "Power Clean",
};
/* Typical working weight (lb) for an average lifter, per exercise. */
static const double BASE_WEIGHT[] = {
    225, 185, 185, 155, 275, 205, 115, 155, 45, 35,
    45, 140, 360, 225, 95, 75, 65, 140, 180, 155,
};
#define NUM_EXERCISES ((int)(sizeof(EXERCISES) / sizeof(EXERCISES[0])))
#define LOG_DAYS 365

static uint64_t rng_state;

static uint64_t rng_next(void) /* xorshift64* */
{
    rng_state ^= rng_state >> 12;
    rng_state ^= rng_state << 25;
    rng_state ^= rng_state >> 27;
    return rng_state * 2685821657736338717ULL;
}

static double rng_unit(void) /* uniform in [0, 1) */
{
    return (double)(rng_next() >> 11) * (1.0 / 9007199254740992.0);
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: %s NUM_SETS NUM_USERS OUTPUT.csv [SEED]\n", argv[0]);
        return 1;
    }
    long long num_sets = atoll(argv[1]);
    int num_users = atoi(argv[2]);
    const char *path = argv[3];
    rng_state = (argc > 4) ? strtoull(argv[4], NULL, 10) : 42ULL;
    if (rng_state == 0) rng_state = 42ULL;
    if (num_sets <= 0 || num_users <= 0) {
        fprintf(stderr, "NUM_SETS and NUM_USERS must be positive\n");
        return 1;
    }

    /* Each lifter gets a strength multiplier so their numbers are consistent. */
    double *strength = malloc((size_t)num_users * sizeof *strength);
    if (!strength) { perror("malloc"); return 1; }
    for (int u = 0; u < num_users; u++)
        strength[u] = 0.55 + 0.9 * rng_unit();

    FILE *out = fopen(path, "wb");
    if (!out) { perror(path); free(strength); return 1; }
    static char iobuf[1 << 20];
    setvbuf(out, iobuf, _IOFBF, sizeof iobuf);

    long start = days_from_civil(LOG_START_YEAR, 1, 1);
    for (long long i = 0; i < num_sets; i++) {
        long day = (long)(i * LOG_DAYS / num_sets);
        int y, m, d;
        civil_from_days(start + day, &y, &m, &d);

        int user = (int)(rng_next() % (uint64_t)num_users);
        int ex = (int)(rng_next() % NUM_EXERCISES);
        int reps = 1 + (int)(rng_next() % 12);

        /* Heavier for low reps, slow progression over the year. */
        double intensity = 1.05 - 0.04 * reps + 0.1 * rng_unit();
        double progress = 1.0 + 0.10 * (double)day / LOG_DAYS;
        double w = BASE_WEIGHT[ex] * strength[user] * intensity * progress;
        long plates = (long)(w / 2.5 + 0.5);
        if (plates < 1) plates = 1;

        fprintf(out, "%d,%04d-%02d-%02d,%s,%.1f,%d\n",
                user + 1, y, m, d, EXERCISES[ex], plates * 2.5, reps);
    }

    fclose(out);
    free(strength);
    return 0;
}
