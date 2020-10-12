#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#define TEST 1
#define PIPE 124

#ifdef TEST
#define DBG(a) if (TEST) {a;}
#else
#define DBG(a)
#endif

#ifdef TEST
#define ASSERT_OK(error) fprintf(stderr, "ERROR AT LINE %d, FUNCTION %s\nCALLING DUMP\n", __LINE__, __func__); \
                         dump(error);  
#else
#define ASSERT_OK(error)                                                                    
#endif  

enum errors {
    NO_INPUT = -1,
    ARGS_OVERFLOW = -2,
    NOFILE = -100,
    READ_SIZE = -4,
    ARGS_UNDERFLOW = -5,
    BAD_ALLOC = -6,
    INP_FILE = -7,
    FORK_ERROR = -8,
    PIPE_ERROR = -9,
    EXECVP_ERROR = -10,
};


struct command_t {
    char** argv;
    int argc;
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

        case FORK_ERROR:
        fprintf(stderr, "pipe error: could not fork\n");
        exit(FORK_ERROR);
        break;

        case EXECVP_ERROR:
        fprintf(stderr, "pipe error: could not exec cmd\n");
        exit(EXECVP_ERROR);
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

unsigned get_cmd_count(char* buf, unsigned buf_size) {
    DBG(fprintf(stderr, "Getting cmd count...\n"))
    char* next = NULL;
    unsigned n_cmd = 1;
    char* p = buf;

     while ((next = strchr(p, '|')) != NULL) {
        DBG(fprintf(stderr, "Next cmd in buf:%s\n", next))
        ++n_cmd;
        p = next;
        ++p;
    }

    DBG(fprintf(stderr, "\n\n\n"))

    return n_cmd;
}

char* skip_spaces(char* buf) {
    assert(buf);
    char* p = buf;
    while (isspace(*p)){
        ++p;
    }
    return p;
}

unsigned get_argc(char* cmd) {
    DBG(fprintf(stderr, "Getting argc in cmd:%s\n", cmd))
    unsigned argc = 1;
    char* next = NULL;
    char* p = cmd;
    while ((next = strchr(p, ' ')) != NULL) {
        p = next;
        p = skip_spaces(p);
        if (*p == '\0') {
            break;
        }
        ++p;
        ++argc;
    }

    return argc;
}

char** get_argv(char* cmd, int argc) {
    char** argv = (char**) calloc(argc + 1, sizeof(char*));
    argv[argc] = NULL;
    if (argv == NULL) {
        ASSERT_OK(BAD_ALLOC)
    }
    char* p = cmd;
    char* next = NULL;
    int i = 0;

    while ((next = strchr(p, ' ')) != NULL) {
        argv[i] = p;
        p = next;
        *next = '\0';
        ++p;
        p = skip_spaces(p);
        ++i;
    } 
    argv[i] = p;

    if ((next = strchr(p, '\n')) != NULL) {
        *next = '\0';
    }

    argv[argc] = NULL;

    return argv;
}

void print_cmd(struct command_t* cmd) {
    for (int i = 0; i < cmd->argc; ++i) {
        fprintf(stderr, "Cmd[%d] = %s\n", i, cmd->argv[i]);
    }
    fprintf(stderr, "\n\n\n");
}

void print_commands(struct commands_t* commands) {
    for(int i = 0; i < commands->n_cmd; ++i) {
        fprintf(stderr, "commands[%d]:\n", i);
        print_cmd(&(commands->commands[i]));
    }
    fprintf(stderr, "\n\n\n");
}
 
struct commands_t* get_cmds(char* buf, unsigned buf_size) {

    unsigned n_cmd = get_cmd_count(buf, buf_size);
    DBG(fprintf(stderr, "Number of pipes(commands):%u\n", n_cmd))

    struct commands_t* commands = (struct commands_t*) calloc(1, sizeof(struct commands_t));
    if (commands == NULL) {
        ASSERT_OK(BAD_ALLOC)
    }
    commands->n_cmd = n_cmd;

    struct command_t* n_comm = (struct command_t*) calloc(commands->n_cmd, sizeof(struct command_t));
    if (n_comm == NULL) {
        ASSERT_OK(BAD_ALLOC)
    }

    unsigned cmd_count = 0;
    char* next = NULL;
    char* p = buf;
    p = skip_spaces(buf);
    DBG(fprintf(stderr, "After skipping spaces:%s\n", p))
    next = strtok(p, "|");
    next = strtok(NULL, "|");
    DBG(fprintf(stderr, "Parsed command:%s\n", p))
    n_comm[cmd_count].argc = get_argc(p);
    DBG(fprintf(stderr, "argc in cmd:%s is:  %d\n", p, n_comm[cmd_count].argc))
    n_comm[cmd_count].argv = get_argv(p, n_comm[cmd_count].argc);
    DBG(print_cmd(&n_comm[cmd_count]))
    DBG(fprintf(stderr, "next: %s\n", next))
    p = next;
    cmd_count++;

    while ((next = strtok(NULL, "|")) != NULL) {
        p = skip_spaces(p);
        DBG(fprintf(stderr, "After skipping spaces:%s\n", p))
        DBG(fprintf(stderr, "Parsed command:%s\n", p))
        n_comm[cmd_count].argc = get_argc(p);
        DBG(fprintf(stderr, "argc in cmd:%s is:%d\n", p, n_comm[cmd_count].argc))
        n_comm[cmd_count].argv = get_argv(p, n_comm[cmd_count].argc);
        DBG(print_cmd(&n_comm[cmd_count]))
        DBG(fprintf(stderr, "next: %s\n", next))
        p = next;
        ++cmd_count;
    }
    p = skip_spaces(p);
    DBG(fprintf(stderr, "After skipping spaces:%s\n", p))
    DBG(fprintf(stderr, "Parsed command:%s\n", p))
    n_comm[cmd_count].argc = get_argc(p);
    DBG(fprintf(stderr, "argc in cmd:%s is:%d\n", p, n_comm[cmd_count].argc))
    n_comm[cmd_count].argv = get_argv(p, n_comm[cmd_count].argc);
    DBG(print_cmd(&n_comm[cmd_count]))
    DBG(fprintf(stderr, "next: %s\n", next))
    p = next;
    ++cmd_count;

    commands->commands = n_comm;

    return commands;
}

void exec_cmd(struct command_t* command) {
    execvp(command->argv[0], command->argv);
    DBG(fprintf(stderr, "execvp error\n"))
    exit(EXECVP_ERROR);
}

void exec_commands(struct commands_t* commands) {

    int* fd = (int*) calloc(commands->n_cmd * 2, sizeof(int)); //pipe
    if (fd == NULL) {
        ASSERT_OK(BAD_ALLOC)
    }

    for (int i = 0; i < commands->n_cmd - 1; ++i) { // opening all needed pipes (n_cmd - 1)
        if (pipe(fd + 2 * i) < 0) {
            ASSERT_OK(PIPE_ERROR)
        }
    }
    int status;
    int pid = 0;
    for (int i = 0; i < commands->n_cmd; ++i) {
        if ((pid = fork()) == 0) {
            if (i != commands->n_cmd - 1) { // if not last command changins stdout to pipe
                DBG(fprintf(stderr, "Changing stdout to pipe\n"))
                if (dup2(fd[(2 * i) + 1], STDOUT_FILENO) < 0) {
                    ASSERT_OK(PIPE_ERROR)
                }
            }
            if (i != 0) { // if not first command
                DBG(fprintf(stderr, "Changing stdin to pipe\n"))
                if (dup2(fd[(2 * i) - 2], STDIN_FILENO) < 0) {
                    ASSERT_OK(PIPE_ERROR)
                }
            }
            for (int j = 0; j < 2 * (commands->n_cmd - 1); j++) {
               close(fd[j]);
            }
            exec_cmd(&(commands->commands[i]));
        }
    }

    for (int j = 0; j < 2 * (commands->n_cmd - 1); j++) {
        //close(fd[2 * (commands->n_cmd - 1) - 1]); // closing last pipe to put data to stdout
    close(fd[j]);
    }

    for(int j = 0; j < 2 * commands->n_cmd; j++) {
            wait(&status);
    }
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

    struct commands_t* commands = get_cmds(buf, input_size); // getting commands array

    DBG(fprintf(stderr, "Freeing memory\n"))
    print_commands(commands);
    
    #ifdef TEST
    char* argvv[5] = {"ls", "-G", "-p", "-l", NULL};
    char** argvvv = commands->commands[0].argv;
    int i = 0;
    while (*argvvv != NULL) {
        fprintf(stderr, "argv[%d] = %s\n", i, commands->commands[0].argv[i]);
        ++i;
        ++argvvv;
    }
    #endif

    exec_commands(commands);

    free(buf);
    return 0;
}