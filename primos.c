#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

long n = 5000000;
long chunk = 5000;
long next = 0;
long total = 0;

pthread_mutex_t mutex_next;
pthread_mutex_t mutex_total;

bool is_prime(long num) {
    if (num <= 1) return false;
    for (long i = 2; i * i <= num; i++) {
        if (num % i == 0) return false;
    }
    return true;
}

void print_progress_bar(int current, int n) {
    system("clear");
    int width = 100;
    int progress = (current * width) / n;
    printf("[");
    for (int i = 0; i < width; i++) {
        if (i < progress) {
            printf("■");
        } else {
            printf(" ");
        }
    }
    printf("]\n");
}

void *work(void *arg) {
    int id = (int)(long)arg;
    printf("Hello from thread %d!\n", id);
    while (true) {
        pthread_mutex_lock(&mutex_next);

        long start = next;
        next += chunk;
        
        pthread_mutex_unlock(&mutex_next);

        if (start >= n) break;

        long end = start + chunk;
        if (end > n) end = n;

        long local_count = 0;
        for (long i = start; i < end; i++) {
            if (is_prime(i)) {
                local_count++;
            }
        }
        pthread_mutex_lock(&mutex_total);

        print_progress_bar(end, n);
        total += local_count;
        
        pthread_mutex_unlock(&mutex_total);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <number of threads>\n", argv[0]);
        return 1;
    }

    int k = atoi(argv[1]);

    pthread_mutex_init(&mutex_next, NULL);
    pthread_mutex_init(&mutex_total, NULL);

    clock_t start = clock();

    pthread_t thread[k];
    for (int i = 0; i < k; i++) {
        pthread_create(&thread[i], NULL, work, (void *)(long)i);
    }
    for (int i = 0; i < k; i++) {
        pthread_join(thread[i], NULL);
    }

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Time taken: %.2f seconds\n", time_spent);
    printf("Total primes up to %ld: %ld\n", n, total);

    pthread_mutex_destroy(&mutex_next);
    pthread_mutex_destroy(&mutex_total);
    return 0;
}