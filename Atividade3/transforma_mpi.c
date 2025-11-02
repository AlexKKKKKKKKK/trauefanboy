// compilar: mpicc -o transforma_mpi transforma_mpi.c
// executar: mpirun -np 5 ./transforma_mpi

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define DATA_SIZE 100  // Tamanho total do vetor de dados

// Função que aplica a transformação: eleva o número ao quadrado
int transform_data(int x) {
    return x * x;
}

int main() {
    int id, P;  // id = rank do processo, P = número total de processos

    MPI_Init(NULL, NULL);  // Inicializa o ambiente MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &id);  // Obtém o rank (ID) do processo atual
    MPI_Comm_size(MPI_COMM_WORLD, &P);   // Obtém o número total de processos

    int elems_per_proc = DATA_SIZE / P;  // Quantidade de elementos por processo

    // Vetor local que armazenará os dados recebidos por cada processo
    int local_data[DATA_SIZE / P];

    int* original_data = NULL;  // Ponteiro para o vetor original (só usado pelo processo 0)
    int* all_transformed_data = (int *)malloc(sizeof(int) * DATA_SIZE);  // Todos terão o vetor completo

    // Processo 0 inicializa os dados originais e aloca memória para o resultado
     if (id == 0) {
        original_data = (int *)malloc(sizeof(int) * DATA_SIZE);
        for (int i = 0; i < DATA_SIZE; ++i) {
            original_data[i] = i + 1;
        }
    }

    // Distribui partes iguais do vetor original para todos os processos
    MPI_Scatter(
        original_data, elems_per_proc, MPI_INT,   // Envia
        local_data, elems_per_proc, MPI_INT,      // Recebe
        0, MPI_COMM_WORLD
    );

    // Aplica a transformação localmente: x -> x²
    for (int i = 0; i < DATA_SIZE / P; ++i) {
        local_data[i] = transform_data(local_data[i]);
    }

    // Usa MPI_Allgather para juntar os dados transformados em TODOS os processos
    MPI_Allgather(
        local_data, elems_per_proc, MPI_INT,           // Envia
        all_transformed_data, elems_per_proc, MPI_INT, // Todos recebem o vetor completo
        MPI_COMM_WORLD
    );

    // Cada processo pode agora imprimir o vetor completo transformado
    printf("Processo %d - Dados transformados: ", id);
    for (int i = 0; i < DATA_SIZE; ++i) {
        printf("%d ", all_transformed_data[i]);
    }
    printf("\n");

    // Libera memória
    free(all_transformed_data);
    if (id == 0) {
        free(original_data);
    }

    MPI_Finalize();  // Finaliza o ambiente MPI
    return 0;
}
