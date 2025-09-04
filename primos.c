#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

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

void print_progress_bar(int current, int n, int k) {
    int width = 50;
    int progress = (current * width) / n;
	
    // Move o cursor para o início da linha
    printf("\rk=%d [", k);

    for (int i = 0; i < width; i++) {
        if (i < progress) {
            printf("■");
        } else {
            printf(" ");
        }
    }
    printf("] %d%%", (current * 100) / n);
    fflush(stdout); // força a atualização imediata
}

struct WorkArgs {
	int id;
	int k;
};

void *work(void *arg) {

    struct WorkArgs* args = (struct WorkArgs*)arg;
    int id = (int)args->id;
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

	    print_progress_bar(end, n,args->k);
        total += local_count;

        pthread_mutex_unlock(&mutex_total);
    }
    return NULL;
}

struct BenchmarkResult
{
    int num_threads;
    double time_spent;
    long total_primes;
};


struct BenchmarkResult benchmark(int num_threads) {
    total = 0;
    next = 0;
    pthread_mutex_init(&mutex_next, NULL);
    pthread_mutex_init(&mutex_total, NULL);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_t thread[num_threads];
    for (int i = 0; i < num_threads; i++) {
		struct WorkArgs args = {i,num_threads};
        pthread_create(&thread[i], NULL, work, (void *)&args);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(thread[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_spent = (end.tv_sec - start.tv_sec) * 1000.0;
    time_spent += (end.tv_nsec - start.tv_nsec) / 1000000.0;

    pthread_mutex_destroy(&mutex_next);
    pthread_mutex_destroy(&mutex_total);

    struct BenchmarkResult result = {num_threads, time_spent, total};
    return result;
}

void print_header(){
    printf("\n");
    printf(" _______________________________________________________\n");
    printf("| k\t| tempo_ms\t| total_primos\t| speedup_vs_k1 |\n");
    printf("|-------|---------------|---------------|---------------|\n");
}

void auto_benchmark(){
    struct BenchmarkResult result[5];
    result[0] = benchmark(1);
    result[1] = benchmark(2);
    result[2] = benchmark(4);
    result[3] = benchmark(6);
    result[4] = benchmark(8);

    print_header();
    for(int i=0;i<=4;i++){
        double speedup = result[0].time_spent / result[i].time_spent;
        printf("| %d\t| %.2f\t| %ld\t| %.2f\t\t|\n", result[i].num_threads, result[i].time_spent, result[i].total_primes, speedup);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <number of threads>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-b") == 0){
        auto_benchmark();
        return 0;
    }

    int k = atoi(argv[1]);
    benchmark(k);
    return 0;

}

