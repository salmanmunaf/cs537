#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("pzip: file1 [file2 ...]\n");
        return 1;
    }
    
    char curr;
    char prev = '\0';
    int count = 1;
    for (int i = 1; i < argc; i++) {
        FILE *inFile;
        inFile = fopen(argv[i], "r");
        if (inFile) {
            if (i == 1) {
                prev = fgetc(inFile);
            }

            while ((curr = fgetc(inFile)) != EOF) {
                if (curr == '\0') {
                    continue;
                }

                // printf("cur: %x\n", curr);
                if (curr != prev) {
                    // printf("count: %d, val: %x\n", count, prev);
                    fwrite(&count, sizeof(int), 1, stdout);
                    fwrite(&prev, sizeof(char), 1, stdout);
                    prev = curr;
                    count = 1;
                } else {
                    count += 1;
                }
            }

            fclose(inFile);
        } else {
            if (i == (argc-1)) {
                // printf("count: %d, val: %x\n", count, prev);
                fwrite(&count, sizeof(int), 1, stdout);
                fwrite(&prev, sizeof(char), 1, stdout);
            }
            continue;
        }
        if (i == (argc-1)) {
            // printf("count: %d, val: %x\n", count, prev);
            fwrite(&count, sizeof(int), 1, stdout);
            fwrite(&prev, sizeof(char), 1, stdout);
        }
    }

    return 0;
}