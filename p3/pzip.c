#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/sysinfo.h>
#include <pthread.h>

typedef struct {
    int count;
    char value;
} encoded_t;

typedef struct {
    char *offset;
    size_t size;
    int file_num;
    int thread_num;
    encoded_t ***output;
} myarg_t;

void *compress_file(void *arg) {
    myarg_t *args = (myarg_t *) arg;
    size_t size = args->size;
    char *offset = args->offset;
    encoded_t ***output = args->output;
    char curr;
    char prev = *offset;
    while (prev == '\0' && size > 0) {
        offset++;
        size--;
        prev = *offset;
    }
    offset++;
    int index = 0;
    int count = 1;
    for (int i = 1; i < size; i++) {
        curr = *offset;
        if (curr == '\0') {
            offset++;
            continue;
        }
        if (curr != prev) {
            encoded_t element = {count, prev};
            output[args->file_num][args->thread_num][index] = element;
            
            count = 1;
            index += 1;
        } else {
            count += 1;
        }
        prev = curr;
        offset++;
    }

    if (prev != '\0') {
        encoded_t element = {count, prev};
        output[args->file_num][args->thread_num][index] = element;
        index += 1;
    }
    
    
    encoded_t last_element = {-1, '#'};
    output[args->file_num][args->thread_num][index] = last_element;
    index += 1;
    return NULL;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("pzip: file1 [file2 ...]\n");
        return 1;
    }

    int NUM_THREADS = get_nprocs();
    int NUM_FILES = argc - 1;
    size_t MAX_SIZE = 999999999;
    encoded_t ***output = (encoded_t ***)malloc(NUM_FILES * sizeof(encoded_t **));
    for (int i = 0; i < NUM_FILES; i++) {
        output[i] = (encoded_t **)malloc(NUM_THREADS * sizeof(encoded_t*));
        for (int j = 0; j < NUM_THREADS; j++) {
            output[i][j] = (encoded_t *)malloc(MAX_SIZE * sizeof(encoded_t));
        }
    }
        
    
    for (int i = 1; i < argc; i++) {
        int inFile = open(argv[i], O_RDONLY);
        if (inFile == -1) {
            for (int j = 0; j < NUM_THREADS; j++) {
                encoded_t last_element = {-1, '#'};
                output[i-1][j][0] = last_element;
            }
            continue;
        }
        
        struct stat file_stat;
        if (fstat(inFile, &file_stat) == -1) {
            printf("pzip: unable to retrieve file stats\n");
            close(inFile);
            return 1;
        }

        size_t file_size = file_stat.st_size;

        if (file_size == 0) {
            close(inFile);
            continue;
        }

        char *data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, inFile, 0);
        if (data == MAP_FAILED) {
            printf("pzip: mmap failed\n");
            close(inFile);
            return 1;
        }

        size_t part = file_size / NUM_THREADS;
        size_t rem = file_size % NUM_THREADS;
        pthread_t threads[NUM_THREADS];
        myarg_t args[NUM_THREADS];
        for (int j = 0; j < NUM_THREADS - 1; j++) {
            args[j] = (myarg_t) { (char *) (data + (j * part)), part, i-1, j, output };
            pthread_create(&threads[j], NULL, compress_file, &args[j]);
        }
        args[NUM_THREADS-1] = (myarg_t) { (char *) (data+((NUM_THREADS-1)*part)), rem + part, i-1, NUM_THREADS-1, output };
        pthread_create(&threads[NUM_THREADS-1], NULL, compress_file, &args[NUM_THREADS-1]);

        for (int j = 0; j < NUM_THREADS; j++) {
            pthread_join(threads[j], NULL);
        }

        close(inFile);
    }

    char prev = output[0][0][0].value;
    char curr;
    int count = output[0][0][0].count;
    int index = 1;
    for (int i = 0; i < NUM_FILES; i++) {
        for (int j = 0; j < NUM_THREADS; j++) {
            if (prev != '#') {
                while ((curr = output[i][j][index].value) != '#') {
                    if (curr == prev) {
                        count += output[i][j][index].count;
                    } else {
                        fwrite(&count, sizeof(int), 1, stdout);
                        fwrite(&prev, sizeof(char), 1, stdout);
                        prev = curr;
                        count = output[i][j][index].count;
                    }
                    index += 1;
                }
                index = 0;
            }
        }
    }
    fwrite(&count, sizeof(int), 1, stdout);
    fwrite(&prev, sizeof(char), 1, stdout);
    return 0;
}