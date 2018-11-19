#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#define READ 0
#define WRITE 1

pid_t popen2(const char *command, int *infp, int *outfp, int *errfp)
{
    int p_stdin[2], p_stdout[2], p_stderr[2];
    pid_t pid;

    if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0 || pipe(p_stderr) != 0)
        return -1;

    pid = fork();

    if (pid < 0)
        return pid;
    else if (pid == 0)
    {
        dup2(p_stdin[READ], STDIN_FILENO);
        dup2(p_stdout[WRITE], STDOUT_FILENO);
	dup2(p_stderr[WRITE], STDERR_FILENO);

     close(p_stdin[READ]);
     close(p_stdin[WRITE]);
     close(p_stdout[READ]);
     close(p_stdout[WRITE]);
     close(p_stderr[READ]);
     close(p_stderr[WRITE]);

     char* params[100];
     memset(params, 0, sizeof(params));
     params[0] = strtok(command, "|");
     int ctr=1;
     do {
          params[ctr] = strtok(NULL, "|");
	  ctr++;
     } while (params[ctr-1] != NULL); 
     	
        execv("/usr/local/bin/ffmpeg", params);
        exit(1);

    }

    close(p_stdin[READ]);
    close(p_stdout[WRITE]);

    if (infp == NULL)
        close(p_stdin[WRITE]);
    else
        *infp = p_stdin[WRITE];

    if (outfp == NULL)
        close(p_stdout[READ]);
    else
        *outfp = p_stdout[READ];

    if (errfp == NULL)
        close(p_stderr[READ]);
    else
        *errfp = p_stderr[READ];

    return pid;
}

int pclose2(pid_t pid) {
    int internal_stat;
    waitpid(pid, &internal_stat, 0);
    return WEXITSTATUS(internal_stat);
}
