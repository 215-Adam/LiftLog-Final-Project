/* Paste into https://godbolt.org  (x86-64 gcc, flags: -O2)
 * Hot loop of baseline.c, self-contained. */
#include <stdlib.h>
#include <string.h>

#define MAX_WEEKS 64

struct SetNode {
    int user_id;
    char *exercise;
    double weight;
    int reps;
    long date;
    struct SetNode *next;
};

struct ExerciseStat {
    char *name;
    double best_e1rm;
    double pr_weight;
    double volume;
    int sets;
    struct ExerciseStat *next;
};

struct UserRec {
    struct ExerciseStat *stats;
    double weekly_volume[MAX_WEEKS];
};

static struct ExerciseStat *find_stat(struct ExerciseStat *list, const char *name)
{
    for (struct ExerciseStat *s = list; s != NULL; s = s->next)
        if (strcmp(s->name, name) == 0)
            return s;
    return NULL;
}

static struct ExerciseStat *new_stat(struct UserRec *user, const char *name)
{
    struct ExerciseStat *s = malloc(sizeof *s);
    s->name = strdup(name);
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

void compute_stats(struct SetNode *head, struct UserRec *users, long start)
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
