//only accepts commands in "cmd1 | cmd 2 | cmd3" format

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h> 
#include <sys/wait.h>

int cmdparser(char* str, char** ptr, int size);
int argparser(int icmd, char** ptr, char** argv);
int execer(int icmd, char** ptr);
void closefd(int fdsarr[][2], int ncmd);

int main() {
    char str[100]; //buffer for source pipeline
    char* ptr[sizeof(str)]; //buffer for cmd beginnings
    int ncmd = cmdparser(str, ptr, sizeof(str));

    //for (int i = 0; i < ncmd; i++) {printf("%s ", ptr[i]);}
    int fdsarr[ncmd - 1][2]; 
    for (int i = 0; i < ncmd - 1; i++) {
        int p = pipe(fdsarr[i]);
        if (p < 0) {
            perror("pipe fail");
            exit(1);
        }
    }

    //printf("im parent, pid %d, ppid %d\n", getpid(), getppid());
    for (int i = 0; i < ncmd; i++) {
        int f = fork();
        if (f == 0) {
            //printf("im child_%d, pid %d, ppid %d\n", i, getpid(), getppid());
            if (i == 0) {
                dup2(fdsarr[i][1], 1);
                closefd(fdsarr, ncmd);
                execer(i, ptr);
            } else if (i == ncmd - 1) {
                dup2(fdsarr[i-1][0], 0);
                closefd(fdsarr, ncmd);
                execer(i, ptr);
            } else {
                dup2(fdsarr[i - 1][0], 0);
                dup2(fdsarr[i][1], 1);
                closefd(fdsarr, ncmd);
                execer(i, ptr);
            }
            return 0;
        }
    }

    closefd(fdsarr, ncmd);
    for (int i = 0; i < ncmd; i++) wait(NULL);
    return 0;
}

int cmdparser(char* str, char** ptr, int size) {
    fgets(str, size, stdin);
    int i = 0;
    int j = 0;
    ptr[j++] = str;
    while (str[i] != '\0') {
        if (str[i] == '|') {
            ptr[j++] = str + i + 2;
            str[i - 1] = '\0';
        }
        i++;
    }
    if (str[i - 1] == '\n') str[i - 1] = '\0';
    return j;
}

int argparser(int icmd, char** ptr, char** argv) {
    int ac = 0;
    int j = 0;
    char *cmdbeg = ptr[icmd];
    while (1) {
        char s[10];
        int len;
        while (cmdbeg[j] == ' ') j++;
        if (cmdbeg[j] == '\0') break;
        sscanf(cmdbeg + j, "%s%n", s, &len);
        argv[ac++] = cmdbeg + j;
        j += len;
        if (cmdbeg[j] != '\0') {
            cmdbeg[j] = '\0';
            j++;
        }
    }
    argv[ac] = NULL;
    return ac;
}

int execer(int icmd, char** ptr) {
    char* argv[10];
    argparser(icmd, ptr, argv);
    execvp(argv[0], argv);
    fprintf(stderr, "execvp in child_%d: %s\n", icmd, strerror(errno));
    return 0;   
}

void closefd(int fdsarr[][2], int ncmd) {
    for (int i = 0; i < ncmd - 1; i++) {
        close(fdsarr[i][0]);
        close(fdsarr[i][1]);
    }
}
