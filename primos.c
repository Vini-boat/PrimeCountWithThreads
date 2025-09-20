#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

long N = 5000000;
long CHUNK = 5000;

int benchmark_k[] = {1,2,4,6,8};
int num_benchmarks = sizeof(benchmark_k) / sizeof(benchmark_k[0]);

// Variáveis globais, serão acessadas via mutex
long next = 0;
long total = 0;

// Criação de um mutex para cada variável
pthread_mutex_t mutex_next;
pthread_mutex_t mutex_total;


bool is_prime(long num) {
    if (num <= 1) return false;
    if (num == 2) return true;
    if (num % 2 == 0) return false;
    
    // i <= sqrt(num)
    // i*i <= num
    // i <= num / i

    // i <= num / i para checar só até a raiz quadrada de num sem perder a precisão
    // i+=2 para pular os pares maiores que 2
    for (long i = 3; i <= num / i; i += 2) {
        if (num % i == 0) return false;
    }
    return true;
}

void print_progress_bar(int current, int n, int k) {
    int width = 50;
    int progress = (current * width) / n;
	
    // Move o cursor para o início da linha
    printf("\rk=%d\t [", k);

    for (int i = 0; i < width; i++) {
        if (i < progress) {
            printf("■");
        } else {
            printf(" ");
        }
    }
    printf("] %d%%", (current * 100) / n);
    fflush(stdout); // força a atualização imediata para não piscar a tela
}

struct WorkArgs {
	int id;
	int k;
};

void *work(void *arg) {

    struct WorkArgs* args = (struct WorkArgs*)arg;
    int id = (int)args->id;
    while (true) {
        pthread_mutex_lock(&mutex_next);

        long start = next;
        next += CHUNK;

        pthread_mutex_unlock(&mutex_next);
        // calcula só até o N
        if (start >= N) break;

        long end = start + CHUNK;
        if (end > N) end = N;

        long local_count = 0;
        for (long i = start; i < end; i++) {
            if (is_prime(i)) {
                local_count++;
            }
        }
        pthread_mutex_lock(&mutex_total);

        // Printa a barra na região crítica para não ter concorrência pelo terminal
	    print_progress_bar(end, N,args->k);
        total += local_count;

        pthread_mutex_unlock(&mutex_total);
    }
    // Caso o ultimo thread a terminar não seja o com o ultimo chunk
    // a barra ficaria em 99%. Por conta disso imprimimos mais uma vez 
    // a barra em 100% no final de cada thread para ter certeza
    print_progress_bar(N, N,args->k);
    return NULL;
}

typedef struct
{
    int num_threads;
    double time_spent;
    long total_primes;
} BenchmarkResult;


BenchmarkResult benchmark(int num_threads) {
    total = 0;
    next = 0;
    pthread_mutex_init(&mutex_next, NULL);
    pthread_mutex_init(&mutex_total, NULL);

    // Usa a clock_gettime para pegar o tempo real, não o tempo de cada thread
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

    // Converte pra milissegundos
    double time_spent = (end.tv_sec - start.tv_sec) * 1000.0;
    time_spent += (end.tv_nsec - start.tv_nsec) / 1000000.0;

    pthread_mutex_destroy(&mutex_next);
    pthread_mutex_destroy(&mutex_total);

    printf("\n"); // para manter as barras de progresso dos outros k na tela
    
    BenchmarkResult result = {num_threads, time_spent, total};
    return result;
}

void print_header(){
    printf("\n");
    printf(" _______________________________________________________\n");
    printf("| k\t| tempo_ms\t| total_primos\t| speedup_vs_k1 |\n");
    printf("|-------|---------------|---------------|---------------|\n");
}

void print_result(BenchmarkResult result){
    printf("| %d\t| %.2f\t| %ld\t| \tN/A\t|\n", result.num_threads, result.time_spent, result.total_primes);
}

void print_footer(){
    printf(" ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯\n");
}

void auto_benchmark(){
    BenchmarkResult result[num_benchmarks];

    for (int i = 0; i < num_benchmarks; i++) {
        result[i] = benchmark(benchmark_k[i]);
    }

    print_header();
    for(int i=0;i<num_benchmarks;i++){
        double speedup = result[0].time_spent / result[i].time_spent *100;
        printf("| %d\t| %.2f\t| %ld\t| %.2f%%\t|\n", result[i].num_threads, result[i].time_spent, result[i].total_primes, speedup);
    }
    print_footer();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <number of threads>\n", argv[0]);
        printf("Usage: %s -b\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-b") == 0){
        auto_benchmark();
        return 0;
    }

    if(atoi(argv[1]) == 0){
        BenchmarkResult result = benchmark(sysconf(_SC_NPROCESSORS_ONLN));
        print_header();
        print_result(result);
        print_footer();
        return 1;
    }

    int k = atoi(argv[1]);
    BenchmarkResult result = benchmark(k);
    print_header();
    print_result(result);
    print_footer();
    return 0;

}

