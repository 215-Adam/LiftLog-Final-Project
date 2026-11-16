/*
 * baseline.c - LiftLog, naive version.
 *
 * Reads a workout log (user_id,YYYY-MM-DD,exercise,weight,reps per line) and
 * reports, for every lifter and exercise: best estimated one-rep max (Epley),
 * personal record (heaviest weight), total volume and set count, plus each
 * lifter's total volume and best training week.
 *
 * This version is deliberately written "the beginner way" (see proposal
 * Section 4):
 *   - fgets one line at a time, split with strtok, convert with atoi/atof
 *   - one malloc per set (SetNode) + one strdup per exercise name
 *   - all sets in one linked list (pushed to the front)
 *   - per-user exercise stats in a linked list searched with strcmp
 *   - free every node one by one at the end
 *
 * Usage:   baseline LOG.csv > report.txt
 * Report goes to stdout; timing and allocation counts go to stderr.
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L /* strdup, clock_gettime */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liftlog_common.h"

/* ---- allocation counting (to show malloc pressure and prove no leaks) ---- */
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

static char *counted_strdup(const char *s)
{
    char *p = strdup(s);
    if (!p) { perror("strdup"); exit(1); }
    malloc_calls++;
    return p;
}

static void counted_free(void *p)
{
    if (p) { free(p); free_calls++; }
}

/* ---- data structures (field order intentionally causes padding) ---- */
struct SetNode {
    int user_id;
    char *exercise;          /* separate malloc for each name */
    double weight;
    int reps;
    long date;               /* days since 1970-01-01 */
    struct SetNode *next;
};

struct ExerciseStat {
    char *name;              /* another strdup */
    double best_e1rm;
    double pr_weight;
    double volume;
    int sets;
    struct ExerciseStat *next;
};

struct UserRec {
    struct ExerciseStat *stats;     /* linked list, searched with strcmp */
    double weekly_volume[MAX_WEEKS];
};

/* ---- parsing ---- */
static long parse_date(const char *s)
{
    int y = 0, m = 0, d = 0;
    if (sscanf(s, "%d-%d-%d", &y, &m, &d) != 3) return -1;
    return days_from_civil(y, m, d);
}

/* Reads the whole log into a linked list. Returns the head; sets *count. */
static struct SetNode *load_log(FILE *f, long *count, int *max_user)
{
    char line[256];
    struct SetNode *head = NULL;
    *count = 0;
    *max_user = -1;

    while (fgets(line, sizeof line, f)) {
        char *tok_user = strtok(line, ",");
        char *tok_date = strtok(NULL, ",");
        char *tok_name = strtok(NULL, ",");
        char *tok_weight = strtok(NULL, ",");
        char *tok_reps = strtok(NULL, ",\r\n");
        if (!tok_user || !tok_date || !tok_name || !tok_weight || !tok_reps)
            continue; /* skip blank / malformed lines */

        struct SetNode *node = counted_malloc(sizeof *node);
        node->user_id = atoi(tok_user);
        node->date = parse_date(tok_date);
        node->exercise = counted_strdup(tok_name);
        node->weight = atof(tok_weight);
        node->reps = atoi(tok_reps);
        node->next = head;
        head = node;

        if (node->user_id > *max_user) *max_user = node->user_id;
        (*count)++;
    }
    return head;
}

/* ---- stats ---- */
static struct ExerciseStat *find_stat(struct ExerciseStat *list, const char *name)
{
    for (struct ExerciseStat *s = list; s != NULL; s = s->next)
        if (strcmp(s->name, name) == 0)
            return s;
    return NULL;
}

static struct ExerciseStat *new_stat(struct UserRec *user, const char *name)
{
    struct ExerciseStat *s = counted_malloc(sizeof *s);
    s->name = counted_strdup(name);
    s->best_e1rm = 0.0;
    s->pr_weight = 0.0;
    s->volume = 0.0;
    s->sets = 0;
    s->next = user->stats;
    user->stats = s;
    return s;
}

static double epley_e1rm(double weight, int reps)
{
    return weight * (1.0 + reps / 30.0);
}

static int week_of(long date, long start)
{
    long w = (date - start) / 7;
    if (w < 0) w = 0;
    if (w >= MAX_WEEKS) w = MAX_WEEKS - 1;
    return (int)w;
}

/* The hot loop: walk every set and update that lifter's stats. */
static void compute_stats(struct SetNode *head, struct UserRec *users, long start)
{
    for (struct SetNode *node = head; node != NULL; node = node->next) {
        if (node->user_id < 0) continue;
        struct UserRec *user = &users[node->user_id];

        struct ExerciseStat *stat = find_stat(user->stats, node->exercise);
        if (stat == NULL)
            stat = new_stat(user, node->exercise);

        double e1rm = epley_e1rm(node->weight, node->reps);
        double vol = node->weight * node->reps;
        if (e1rm > stat->best_e1rm) stat->best_e1rm = e1rm;
        if (node->weight > stat->pr_weight) stat->pr_weight = node->weight;
        stat->volume += vol;
        stat->sets++;
        user->weekly_volume[week_of(node->date, start)] += vol;
    }
}

/* ---- report ---- */
static int cmp_stat_name(const void *a, const void *b)
{
    const struct ExerciseStat *sa = *(const struct ExerciseStat *const *)a;
    const struct ExerciseStat *sb = *(const struct ExerciseStat *const *)b;
    return strcmp(sa->name, sb->name);
}

static void print_summary(FILE *out, struct UserRec *users, int max_user)
{
    for (int u = 0; u <= max_user; u++) {
        struct UserRec *user = &users[u];
        if (user->stats == NULL) continue;

        double total = 0.0, best = -1.0;
        int best_week = 0;
        for (int w = 0; w < MAX_WEEKS; w++) {
            total += user->weekly_volume[w];
            if (user->weekly_volume[w] > best) { best = user->weekly_volume[w]; best_week = w; }
        }
        fprintf(out, "user %d: total_volume=%.1f best_week=%d week_volume=%.1f\n",
                u, total, best_week + 1, best);

        /* Sort this user's exercises by name so output order is stable. */
        int n = 0;
        for (struct ExerciseStat *s = user->stats; s; s = s->next) n++;
        struct ExerciseStat **arr = counted_malloc((size_t)n * sizeof *arr);
        int i = 0;
        for (struct ExerciseStat *s = user->stats; s; s = s->next) arr[i++] = s;
        qsort(arr, (size_t)n, sizeof *arr, cmp_stat_name);
        for (i = 0; i < n; i++)
            fprintf(out, "  %s: e1rm=%.2f pr=%.1f volume=%.1f sets=%d\n",
                    arr[i]->name, arr[i]->best_e1rm, arr[i]->pr_weight,
                    arr[i]->volume, arr[i]->sets);
        counted_free(arr);
    }
}

/* ---- cleanup ---- */
static void free_all(struct SetNode *head, struct UserRec *users, int max_user)
{
    while (head) {
        struct SetNode *next = head->next;
        counted_free(head->exercise);
        counted_free(head);
        head = next;
    }
    for (int u = 0; u <= max_user; u++) {
        struct ExerciseStat *s = users[u].stats;
        while (s) {
            struct ExerciseStat *next = s->next;
            counted_free(s->name);
            counted_free(s);
            s = next;
        }
    }
    counted_free(users);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s LOG.csv > report.txt\n", argv[0]);
        return 1;
    }
    FILE *f = fopen(argv[1], "r");
    if (!f) { perror(argv[1]); return 1; }

    long start = days_from_civil(LOG_START_YEAR, 1, 1);
    long count;
    int max_user;

    double t0 = now_ms();
    struct SetNode *head = load_log(f, &count, &max_user);
    double t1 = now_ms();
    fclose(f);

    struct UserRec *users = counted_calloc((size_t)(max_user + 1), sizeof *users);
    double t2 = now_ms();
    compute_stats(head, users, start);
    double t3 = now_ms();

    print_summary(stdout, users, max_user);
    fflush(stdout);

    double t4 = now_ms();
    free_all(head, users, max_user);
    double t5 = now_ms();

    double load_ms = t1 - t0, process_ms = t3 - t2, cleanup_ms = t5 - t4;
    fprintf(stderr,
            "baseline: sets=%ld load_ms=%.2f process_ms=%.2f cleanup_ms=%.2f "
            "total_ms=%.2f mallocs=%ld frees=%ld\n",
            count, load_ms, process_ms, cleanup_ms,
            load_ms + process_ms + cleanup_ms, malloc_calls, free_calls);
    return 0;
}
