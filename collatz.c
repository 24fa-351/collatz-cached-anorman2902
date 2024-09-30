#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_RANDOM_VALUE 1000000
#define MIN_RANDOM_VALUE 1

typedef struct CacheEntry {
    int number;
    int steps;
    struct CacheEntry *next;
    struct CacheEntry *previous;
} CacheEntry;

typedef struct Cache {
    CacheEntry *head;
    CacheEntry *tail;
    int size;
    int max_size;
} Cache;

// Function declarations
int calculate_collatz_steps(int starting_number);
int get_steps_with_cache(Cache *cache, int number, int (*eviction_policy)(Cache *), int *cache_hit);
Cache *create_cache(int max_size);
void free_cache(Cache *cache);
int lru_eviction(Cache *cache);
int fifo_eviction(Cache *cache);
int random_eviction(Cache *cache);
int cache_contains(Cache *cache, int number);
void cache_insert(Cache *cache, int number, int steps, int (*eviction_policy)(Cache *));
void cache_remove_oldest(Cache *cache);
void cache_move_to_tail(Cache *cache, CacheEntry *entry);
double calculate_cache_hit_percentage(int cache_hits, int total_tests);


int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <N> <MIN> <MAX> <CACHE_POLICY> <CACHE_SIZE>\n", argv[0]);
        return 1;
    }

    int number_of_tests;
    int min_value;
    int max_value;
    int cache_size;
    char cache_policy[10];

    if (sscanf(argv[1], "%d", &number_of_tests) != 1 ||
        sscanf(argv[2], "%d", &min_value) != 1 ||
        sscanf(argv[3], "%d", &max_value) != 1 ||
        sscanf(argv[5], "%d", &cache_size) != 1 ||
        sscanf(argv[4], "%s", cache_policy) != 1) {
        fprintf(stderr, "Error: Invalid input format. Please provide integers for N, MIN, MAX, a string for CACHE_POLICY, and CACHE_SIZE.\n");
        return 1;
    }

    // Validating input values
    if (number_of_tests <= 0 || min_value < MIN_RANDOM_VALUE || max_value <= min_value) {
        fprintf(stderr, "Error: Invalid values. Ensure N > 0, MIN >= %d, and MAX > MIN.\n", MIN_RANDOM_VALUE);
        return 1;
    }

    int (*eviction_policy)(Cache *) = NULL;
    if (strcmp(cache_policy, "LRU") == 0) {
        eviction_policy = lru_eviction;
    } else if (strcmp(cache_policy, "FIFO") == 0) {
        eviction_policy = fifo_eviction;
    } else if (strcmp(cache_policy, "RR") == 0) {
        eviction_policy = random_eviction;
    } else {
        fprintf(stderr, "Error: Unsupported cache policy! Use LRU, FIFO, or RR.\n");
        return 1;
    }

    srand(time(NULL));
    Cache *cache = create_cache(cache_size);
    int total_cache_hits = 0;

    // For loop set up to output in the form of a data table
    printf("%15s %10s %10s\n", "Random Number", "Steps", "Cache Hit");
    printf("%15s %10s %10s\n", "-------------", "------", "---------");

    for (int test_index = 0; test_index < number_of_tests; test_index++) {
        int random_number = (rand() % (max_value - min_value + 1)) + min_value;
        int cache_hit = 0;
        int steps_to_one = get_steps_with_cache(cache, random_number, eviction_policy, &cache_hit);
        total_cache_hits += cache_hit;
        printf("%15d %10d %10s\n", random_number, steps_to_one, cache_hit ? "Yes" : "No"); 
    }

    double hit_percentage = calculate_cache_hit_percentage(total_cache_hits, number_of_tests);
    printf("\nCache Hit Percentage: %.2f%%\n", hit_percentage);

    free_cache(cache);

    return 0;
}

int calculate_collatz_steps(int starting_number) {
    int step_count = 0;

    while (starting_number != 1) {
        if (starting_number % 2 == 0) {
            starting_number /= 2;
        } else {
            starting_number = 3 * starting_number + 1;
        }
        step_count++;
    }
    return step_count;
}

int get_steps_with_cache(Cache *cache, int number, int (*eviction_policy)(Cache *), int *cache_hit) {
    int cached_steps = cache_contains(cache, number);
    if (cached_steps != -1) {
        *cache_hit = 1;
        return cached_steps;
    }

    *cache_hit = 0;
    int steps = calculate_collatz_steps(number);
    cache_insert(cache, number, steps, eviction_policy);
    return steps;
}

Cache *create_cache(int max_size) {
    Cache *cache = (Cache *)malloc(sizeof(Cache));
    cache->head = cache->tail = NULL;
    cache->size = 0;
    cache->max_size = max_size;
    return cache;
}

void free_cache(Cache *cache) {
    CacheEntry *current = cache->head;
    while (current != NULL) {
        CacheEntry *next = current->next;
        free(current);
        current = next;
    }
    free(cache);
}

int lru_eviction(Cache *cache) {
    cache_remove_oldest(cache);
    return 0;
}

int fifo_eviction(Cache *cache) {
    cache_remove_oldest(cache);
    return 0;
}

int random_eviction(Cache *cache) {
    if (cache->size == 0) {
        return 0;
    }

    int random_index = rand() % cache->size;
    CacheEntry *current = cache->head;

    for (int i = 0; i < random_index; i++) {
        current = current->next;
    }

    if (current == cache->head) {
        cache->head = current->next;
        if (cache->head != NULL) {
            cache->head->previous = NULL;
        }
    } else {
        current->previous->next = current->next;
        if (current->next != NULL) {
            current->next->previous = current->previous;
        }
    }

    if (current == cache->tail) {
        cache->tail = current->previous;
    }

    free(current);
    cache->size--;
    return 0;
}


int cache_contains(Cache *cache, int number) {
    CacheEntry *current = cache->head;
    while (current != NULL) {
        if (current->number == number) {
            cache_move_to_tail(cache, current);
            return current->steps;
        }
        current = current->next;
    }
    return -1;
}

void cache_insert(Cache *cache, int number, int steps, int (*eviction_policy)(Cache *)) {
    if (cache->size == cache->max_size) {
        eviction_policy(cache);
    }

    CacheEntry *new_entry = (CacheEntry *)malloc(sizeof(CacheEntry));
    new_entry->number = number;
    new_entry->steps = steps;
    new_entry->next = NULL;

    if (cache->head == NULL) {
        cache->head = cache->tail = new_entry;
    } else {
        cache->tail->next = new_entry;
        cache->tail = new_entry;
    }
    cache->size++;
}

void cache_remove_oldest(Cache *cache) {
    if (cache->head == NULL) {
        return;
    }

    CacheEntry *old_entry = cache->head;
    cache->head = cache->head->next;
    free(old_entry);
    cache->size--;
}

void cache_move_to_tail(Cache *cache, CacheEntry *entry) {
    if (cache->tail == entry) {
        return;
    }

    if (entry->previous != NULL) {
        entry->previous->next = entry->next;
    } else {
        cache->head = entry->next;
    }

    if (entry->next != NULL) {
        entry->next->previous = entry->previous;
    } else {
        cache->tail = entry->previous;
    }

    entry->next = NULL;
    entry->previous = cache->tail;

    if (cache->tail != NULL) {
        cache->tail->next = entry;
    }
    cache->tail = entry;
}

double calculate_cache_hit_percentage(int cache_hits, int total_tests) {
    return ((double)cache_hits / total_tests) * 100;
}