#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

typedef struct Dict {
	int key;
	char *value;
} Dict;

size_t dbSize;
Dict *database;

int find(int key) {
	if (database == NULL) return -1;
	for (int i = 0; i < dbSize; i++) {
		if (database[i].key == key) return i;
	}
	return -1;
}

void put(int key, char *value) {
	int index = find(key);
	if (index != -1) {
		database[index].key = key;
		database[index].value = value;
		return;
	}
	dbSize += 1;
	database = realloc(database, dbSize * sizeof(Dict));
	if (database == NULL) {
		printf("Array not allocated\n");
		exit(1);
	}
	database[dbSize - 1].key = key;
	database[dbSize - 1].value = value;
	return;
}

void get(int key) {
	int index = find(key);
	if (index == -1) {
		printf("%d not found\n", key);
		return;
	}
	printf("%d,%s\n", database[index].key, database[index].value);
	return;
}

void delete(int key) {
	if (database == NULL) return;
	int index = find(key);
	if (index == -1) {
		printf("%d not found\n", key);
		return;
	}
	for (int i = index; i < dbSize - 1; i++) {
		database[i] = database[i + 1];
	}
	dbSize -= 1;
	if (dbSize == 0) {
		free(database);
		database = NULL;
		return;
	}
	database = realloc(database, dbSize * sizeof(Dict));
	if (database == NULL) {
		printf("Array not allocated\n");
		exit(1);
	}
	return;
}

void all() {
	if (database == NULL) return;
	for (int i = 0; i < dbSize; i++) {
		printf("%d,%s\n", database[i].key, database[i].value);
	}
	return;
}

void clear() {
	free(database);
	dbSize = 0;
	database = NULL;
	return;
}

void write() {
	FILE *outFile;
	outFile = fopen("database.txt","w");
	if (!outFile) {
		printf("Error in opening output file");
		exit(2);
	}
	for (int i = 0; i < dbSize; i++) {
		fprintf(outFile, "%d,%s\n", database[i].key, database[i].value);
	}
	fclose(outFile);
	return;
}

void read() {
	FILE *inFile;
	inFile = fopen("database.txt", "r");
	if (inFile) {
		char *contents = NULL;
		size_t len = 0;
		while (getline(&contents, &len, inFile) != -1){
			char *tokens[2];
			for (int i = 0; i < 2; i++) {
				char *found = strsep(&contents, ",");
				tokens[i] = found;
			}
			char *value = tokens[1];
			value[strlen(value)-1] = '\0';
			put(atoi(tokens[0]), value);
		}
		fclose(inFile);
	}
	return;
}

int main(int argc, char *argv[]) {
	dbSize = 0;
	database = NULL;
	read();
	for (int i = 1; i < argc; i++) {
		char *tokens[3] = {'\0'};
		int j = 0;
		char *found;
		while((found = strsep(&argv[i], ",")) != NULL ) {
			if (j > 2) {
				break;
			}
			tokens[j] = found;
			j += 1;
		}

		if (strcmp(tokens[0], "p") == 0) {
			if (tokens[1] == NULL || tokens[2] == NULL) {
				printf("bad command\n");
				continue;
			}
			int key = atoi(tokens[1]);
			if (key == 0) {
				printf("bad command\n");
				continue;
			}
			char *value = tokens[2];
			put(key, value);
		} else if (strcmp(tokens[0], "g") == 0) {
			if (tokens[1] == NULL || tokens[2] != NULL) {
				printf("bad command\n");
				continue;
			}
			int key = atoi(tokens[1]);
			if (key == 0) {
				printf("bad command\n");
				continue;
			}
			get(key);
		} else if (strcmp(tokens[0], "d") == 0) {
			if (tokens[1] == NULL || tokens[2] != NULL) {
				printf("bad command\n");
				continue;
			}
			int key = atoi(tokens[1]);
			if (key == 0) {
				printf("bad command\n");
				continue;
			}
			delete(key);
		} else if (strcmp(tokens[0], "c") == 0) {
			clear();
		} else if (strcmp(tokens[0], "a") == 0) {
			all();
		} else {
			printf("bad command\n");
		}

	}

	write();
	free(database);
	return 0;
}
