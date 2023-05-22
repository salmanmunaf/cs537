#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <ctype.h>

typedef struct DynArr
{
    char **data;
    size_t size;
} DynArr;

// Appends an element to dynamic array
void append(struct DynArr *arry, char* element) {
    arry->size += 1;
    arry->data = realloc(arry->data, arry->size * sizeof(DynArr));
    if (arry->data == NULL) {
		printf("Array not allocated\n");
		exit(EXIT_FAILURE);
	}
    arry->data[arry->size-1] = element;
    return;
}

// Clears the dynamic array
void clear(struct DynArr *arry) {
    free(arry->data);
    arry->size = 0;
    arry->data = NULL;
}

//Source: https://stackoverflow.com/a/122721
// This function removes leading and trailing whitespace from string
char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

//Source: https://stackoverflow.com/a/25736212
// This function adds space around redirection operator
char* process_redirection(char* line) {
    char* string = strdup(line);
    int newlen = 1;

    for (char* p = string; *p; p++)
        newlen += *p == '>' ? 3 : 1;

    char *newstr = malloc (sizeof (char) * newlen);

    for (char *p = string, *q = newstr;; p++)
        if (*p == '>')
        {
            *q++ = ' ';
            *q++ = '>';
            *q++ = ' ';
        }
        else if (!(*q++ = *p))
            break;

    return newstr;
}

// Splits a line into words by breaking it using space delimiter
void parse_line(struct DynArr *arry, char* line) {
    char *string = strdup(line);
    string = trimwhitespace(string);
    string = process_redirection(string);

    char *token = strtok(string, " ");
    // loop through the string to extract all other tokens
    while (token != NULL) {
        if (strlen(token) > 0) {
            token[strcspn(token, "\n")] = '\0';
            append(arry, token);
            token = strtok(NULL, " ");
        }
    }
}

void throw_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void n_exit(int size) {
    if (size > 1) {
        throw_error();
        return;
    }

    exit(EXIT_SUCCESS);
}

void n_cd(struct DynArr *arry) {
    if (arry->size == 1 || arry->size > 2) {
        throw_error();
        return;
    }
    
    int isChanged = chdir(arry->data[1]);
    if (isChanged < 0) {
        throw_error();
    }
}

// Loops over paths to find the binary and launches it
void launch_binary(struct DynArr *paths, char** args) {
    bool binaryFound = false;
    for (int i = 0; i < paths->size; i++) {
        char* path = strdup(paths->data[i]);
        strcat(path, "/");
        strcat(path, args[0]);
        if (access(path, X_OK) == 0) {
            binaryFound = true;
            execv(path, args);
            throw_error();
            return;
        }
    }

    if (binaryFound == false) {
        throw_error();
        exit(EXIT_SUCCESS);
    }
    return;
}

// Returns a count of redirection operators present in the array
int check_redirection(struct DynArr *arry) {
    int redirection_operator_count = 0;
    for (int i = 0; i < arry->size; i++) {
        if (strcmp(arry->data[i], ">") == 0) {
            redirection_operator_count += 1;
        }
    }
    return redirection_operator_count;
}

// Parses line and converts it into a dynamic array of words. It then
// uses that array to run the appropriate command. If its not a 
// builtin command it will launch a child process and wait for it
// to execute that binary
void execute_shell_command(struct DynArr *paths, char *line) {
    DynArr command = {NULL, 0};

    parse_line(&command, line);

    if (command.size == 0) {
        clear(&command);
        return;
    }

    // check if its a built in command
    if (strcmp(command.data[0], "exit") == 0) {
        n_exit(command.size);
    } else if (strcmp(command.data[0], "cd") == 0) {
        n_cd(&command);
    } else if (strcmp(command.data[0], "path") == 0) {
        clear(paths);
        for (int i = 1; i < command.size; i++) {
            append(paths, command.data[i]);
        }
    } else if (strcmp(command.data[0], "loop") == 0) {
        if (command.data[1] == NULL) {
            throw_error();
            clear(&command);
            return;
        }
        int loop_limit = atoi(command.data[1]);
        if (loop_limit < 1) {
            throw_error();
            clear(&command);
            return;
        }

        char *args[command.size - 1];
        for (int i = 2; i < command.size; i++) {
            args[i-2] = command.data[i];
        }
        args[command.size - 2] = NULL;

        int rc = -1;
        for (int i = 0; i < loop_limit; i++) {
            rc = fork();

            if (rc < 0) {
                // fork failed
                throw_error();
            }
            else if (rc == 0) {
                for (int j = 0; j < command.size - 2; j++) {
                    if (strcmp(args[j], "$loop") == 0) {
                        char iteration[50];
                        sprintf(iteration, "%d", i+1);
                        args[j] = iteration;
                    }
                }

                launch_binary(paths, args);
            } else {
                // parent goes down this path (main)
                waitpid(rc, NULL, 0);
            }
        }

    } else {
        int redirection_operator_count = check_redirection(&command);
        if (redirection_operator_count > 1) {
            throw_error();
            clear(&command);
            return;
        }

        if (redirection_operator_count == 1 && (command.size < 3 || strcmp(command.data[command.size - 2], ">") != 0)) {
            throw_error();
            clear(&command);
            return;
        }

        char *args[command.size + 1];
        if (redirection_operator_count) {
            for (int i = 0; i < command.size-2; i++) {
                args[i] = command.data[i];
            }
            args[command.size - 2] = NULL;
        } else {
            for (int i = 0; i < command.size; i++) {
                args[i] = command.data[i];
            }
            args[command.size] = NULL;
        }

        int rc = fork();
        if (rc < 0) {
            // fork failed
            throw_error();
            clear(&command);
            return;
        }
        else if (rc == 0) {

            if (redirection_operator_count) {
                // child: redirect standard output to a file
                close(STDOUT_FILENO);
                open(command.data[command.size - 1], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
            }
            launch_binary(paths, args);
        } else {
            // parent goes down this path (main)
            wait(NULL);
        }
    }
    clear(&command);
}

int main(int argc, char *argv[]) {
    DynArr paths = {NULL, 0};
    char* INITIAL_PATH = "/bin";
    append(&paths, INITIAL_PATH);

    if (argc == 2) {
        // batch mode
        FILE *inFile;
        inFile = fopen(argv[1], "r");
        if (inFile) {
            char line [100];

            while (fgets (line, sizeof(line), inFile) != NULL)
	        {
                if (strlen(line) > 0) {
                    execute_shell_command(&paths, line);
                }
	        }

            fclose(inFile);
        } else {
            throw_error();
            clear(&paths);
            exit(EXIT_FAILURE);
        }
    } else if (argc == 1) {
        //interactive mode
        while(1) {
            printf("wish> ");
            char *line = NULL;
            size_t len = 0;
            getline(&line, &len, stdin);

            execute_shell_command(&paths, line);
            
            free(line);
        }
    } else {
        throw_error();
        clear(&paths);
        exit(EXIT_FAILURE);
    }

    clear(&paths);
    exit(EXIT_SUCCESS);
}