#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define TEST 1
#define PIPE 124

#define DBG(a) if (TEST) {a;}
#ifdef TEST
#define ASSERT_OK(error) fprintf(stderr, "ERROR AT LINE %d, FUNCTION %s\nCALLING DUMP\n", __LINE__, __func__); \
                         dump(error);                                                                          
#endif  

enum errors {
    NO_INPUT = -1,
    ARGS_OVERFLOW = -2,
    NOFILE = -100,
    READ_SIZE = -4,
    ARGS_UNDERFLOW = -5,
    BAD_ALLOC = -6,
    INP_FILE = -7,
};


struct command_t {
    char* cmd;
    char** argv;
    unsigned n_argv;
};

struct commands_t {
    struct command_t* commands;
    unsigned n_cmd;
};

int check_input(int argc, char** argv) {
    if (argc == 1) {
        return NO_INPUT;
    }
    return 0;
}

void dump(int error_type) {
    switch(error_type) {

        case NO_INPUT:
        fprintf(stderr, "pipe error: at least 1 argument expected\n");
        exit(NO_INPUT);
        break;

        case ARGS_OVERFLOW:
        fprintf(stderr, "pipe error: too many arguments passed\n");
        exit(ARGS_OVERFLOW);
        break;

        case EISDIR:
        fprintf(stderr, "pipe error: trying to open a directory\n");
        exit(EISDIR);
        break;

        case ARGS_UNDERFLOW:
        fprintf(stderr, "pipe error: output file does not exist\n");
        exit(ARGS_UNDERFLOW);
        break;

        case BAD_ALLOC:
        fprintf(stderr, "pipe error: could not allocate memory\n");
        exit(BAD_ALLOC);
        break;

        case INP_FILE:
        fprintf(stderr, "pipe error: could not open file\n");
        exit(INP_FILE);
        break;

        case 0:
        break;

        default:
        fprintf(stderr, "pipe error: unexpected error\n");
        exit(-100);
        break;
    }
}

int handle_input(int argc, char** argv) {
    int input = open(argv[1], O_RDONLY);
    if (input < 0) {
        ASSERT_OK(INP_FILE)
    }
    return input;
}


struct commands_t* get_cmd(char* buf, unsigned buf_size) {
    char* next = NULL;
    unsigned n_cmd = 1;
    char* p = buf;
    while ((next = strchr(p, '|')) != NULL) {
        DBG(fprintf(stderr, "next pipe in buf:%s\n", next))
        ++n_cmd;
        p = next;
        ++p;
    }

    DBG(fprintf(stderr, "Number of pipes:%u\n", n_cmd))

    struct command_t* n_commands = (struct command_t*) calloc(n_cmd, sizeof(struct command_t));
    assert(n_commands);

    next = strtok(buf, "|");
    char* next_argv = NULL;
    unsigned cmd_size = 0;
    unsigned count = 0;
    do {
        n_commands[count].cmd = next;
        next = strtok(NULL, "|");
        count++;
    } while (next != NULL);

    struct commands_t* commands = (struct commands_t*) calloc(1, sizeof(struct commands_t));
    assert(commands);
    commands->commands = n_commands;
    commands->n_cmd = n_cmd;

    for (int i = 0; i < n_cmd; ++i) {
        fprintf(stderr, "commands [%d] =%s\n", i, commands->commands[i].cmd);
    }

    for (int i = 0; i < commands->n_cmd; ++i) {
        do {
            n_commands[count].cmd = next;
            next = strtok(NULL, " ");
            count++;
        } while (next != NULL);
    }

    return commands;
}


int main (int argc, char** argv) {
    int result = check_input(argc, argv);
    dump(result);
    int input = handle_input(argc, argv);

    struct stat inp;
    fstat(input, &inp);
    unsigned input_size = inp.st_size;

    char* buf = (char*) calloc(input_size + 1, sizeof(char));
    if (buf == NULL) {
        ASSERT_OK(BAD_ALLOC);
    }

    int n_read = read(input, buf, input_size);

    if (n_read == -1) {
        ASSERT_OK(errno)
    }

    assert(n_read == input_size);

    if (n_read != input_size) {
        ASSERT_OK(READ_SIZE)
    }

    buf[input_size] = '\0';
    DBG(fprintf(stderr, "Buffer:%s\n", buf))

    struct commands_t* commands = get_cmd(buf, input_size); // getting commands array

    DBG(fprintf(stderr, "Freeing memory\n"))

    free(commands->commands);
    free(commands);
    free(buf);
    return 0;
}