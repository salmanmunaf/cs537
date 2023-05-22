#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("wunzip: file1 [file2 ...]\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        FILE *inFile;
        inFile = fopen(argv[i], "r");
        if (inFile) {
            char curr;
            int count;

            while(1) {
                if (fread(&count, sizeof(int), 1, inFile) != 1) {
                    break;
                }

                if (fread(&curr, sizeof(char), 1, inFile) != 1) {
                    break;
                }

                for (int j = 0; j < count; j++) {
                    printf("%c", curr);
                }
            }
            fclose(inFile);
        } else {
            printf("wunzip: unable to open file\n");
            return 1;
        }
    }

    return 0;
}