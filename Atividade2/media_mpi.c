#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

// Função que cria um vetor com valores aleatórios entre 0 e 1
float *create_random_numbers(int num_elements) {
    float *random_numbers = (float *)malloc(sizeof(float) * num_elements);
    for (int i = 0; i < num_elements; ++i) {
        random_numbers[i] = (rand() / (float)RAND_MAX);  // número aleatório entre 0 e 1
    }
    return random_numbers;
}

int main(int argc, char* argv[]) {

    int num_elems_local_array = atoi(argv[1]);  // Converte o argumento para inteiro
    int rank, number_processes;
    float local_sum = 0;     // Soma dos elementos do processo atual
    float global_sum = 0;    // Soma total de todos os processos

    MPI_Init(NULL, NULL);  // Inicializa o ambiente MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // Obtém o rank do processo atual
    MPI_Comm_size(MPI_COMM_WORLD, &number_processes);  // Obtém o total de processos

    // Gera números aleatórios únicos para cada processo (semente baseada no rank)
    srand(time(NULL) * rank);
    float *random_numbers = create_random_numbers(num_elems_local_array);

    // Calcula a soma local dos elementos
    for (int i = 0; i < num_elems_local_array; ++i) {
        local_sum += random_numbers[i];
    }

    float local_avg = local_sum / num_elems_local_array;

    // Reduz as somas locais em uma soma global no processo 0
    MPI_Reduce(&local_sum, &global_sum, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

    float *all_sums = NULL;
    float *all_avgs = NULL;

    if (rank == 0) {
        all_sums = (float *)malloc(number_processes * sizeof(float));
        all_avgs = (float *)malloc(number_processes * sizeof(float));
    }

    // Coleta as somas e médias locais no rank 0
    MPI_Gather(&local_sum, 1, MPI_FLOAT, all_sums, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Gather(&local_avg, 1, MPI_FLOAT, all_avgs, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // O processo 0 imprime os resultados locais e calcula e exibe a média global
    if (rank == 0) {
        for (int i = 0; i < number_processes; ++i) {
            printf("Soma local do processo %d = %f, média = %f\n",
                   i, all_sums[i], all_avgs[i]);
        }

        printf("Soma total = %f, média global = %f\n",
               global_sum, global_sum / (num_elems_local_array * number_processes));
        free(all_sums);
        free(all_avgs);
    }

    free(random_numbers);  // Libera a memória alocada

    MPI_Barrier(MPI_COMM_WORLD);  // Sincroniza todos os processos antes de finalizar
    MPI_Finalize();  // Finaliza o ambiente MPI

    return 0;
}
