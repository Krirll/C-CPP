#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <malloc.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

typedef struct max {
  char name[1024];
}max;

typedef struct listPid {
    pid_t id;
    struct listPid * next;
}list;

int cleanList(list * first);
int addPid(list * first, pid_t pid);
int killPid(list * first);
pid_t lastPid(list * first);
max searchMaxLenFile(char dir[1024]);
max openDir(char dir[1024], pid_t * pid);

int addPid(list * first, pid_t pid) {
    int res = 0;
    while (first->next != 0) first = first->next;
    first->next = (list*) malloc(sizeof(list));
    if (first->next != 0) {
        first->next->id = pid;
        first->next->next = 0;
    } else res = -1;
    return res;
}

int killPid(list * first) {
    list * save = first;
    for(;first->next != 0; save = first, first = first->next);
	free(save->next);
    save->next = 0;
	return save->next == 0 ? 0 : 1;
}

int cleanList(list * first) {
    int res = 0;
    for (;first->next != 0;) res = killPid(first);
    first->id = 0;
    return res;
}

pid_t lastPid(list * first) {
    while (first->next != 0) first = first->next;
    return first->id;
}

max searchMaxLenFile(char dir[1024]) {
    max res;
    int err = 0;
    memset(&res,0,sizeof(res));
    DIR * direct;
    if ((direct = (opendir(dir))) != NULL) {
        struct dirent * item = NULL;
        do {
            errno = 0;
            if ((item = readdir(direct)) != NULL) {
                if (item->d_type == 8) {
                    if (strlen(res.name) < strlen(item->d_name)) strcpy(res.name, item->d_name);
                }
            } else if (item == NULL && errno != 0) {
                perror("erorr while reading");
                err = -1;
            }
        } while (item != NULL && err != -1);
    } else perror("error while read dir in search");
    if (closedir(direct) != 0) perror("error close dir");
    printf("max len in search = %s\n", res.name);
    return res;
}

max openDir(char dir[1024], pid_t * pid) {
    int err = 0, mainParent = 1;
    int fd[2];
    fd[0]=0;
    fd[1]=0;
    char name[1024] = "";
    strcpy(name, dir);
    max res2, res, final;
    DIR * direct = NULL;
    if (pipe(fd) == -1) perror("error pipe");
    else {
        list * first = (list*)malloc(sizeof(list));
        if (first != 0) {
            first->id = -2;
            first->next = 0;
            if ((direct = opendir(dir)) != NULL) {
                struct dirent * item = NULL;
                do {
                    errno = 0;
                    printf("dir = %s\n", name);
                    res = searchMaxLenFile(name);
                    if ((item = readdir(direct)) != NULL) {
                        if (item->d_type == 4 && 
                        strcmp(item->d_name, ".") != 0 && strcmp(item->d_name, "..") != 0) {
                            *pid = fork();
                            if (addPid(first, *pid) == -1) {
                                perror("error while add");
                                err = -1;
                            }
                            if (*pid == 0) { //child process
                                    mainParent = 0;
                                    strcat(name, item->d_name);
                                    strcat(name, "/");
                                    err = closedir(direct);
                                    if (err != 0) {
                                        err = -1;
                                        perror("Error of closing directory");
                                    }
                                    if ((direct = opendir(name)) == NULL) perror("error of opening new dir");
                                    res = searchMaxLenFile(name);
                                    cleanList(first);
                            } else if (*pid == -1) {
                                perror("error of create process");
                                err = -1;
                            } 
                        }
                    } else if (item == NULL && errno != 0) {
                        perror("error while read dir");
                        err = -1;
                    } 
                } while (item != NULL && err != -1);
            } else perror("error of opening directory");
            
            //wait here
            if (*pid > 0) {
                int status;
                //Здесь надо дождаться завершения дочернего процесса
                for (; lastPid(first) != 0 && lastPid(first) != -2;) {
                    printf("%s\n", res.name);
                    pid_t result_of_waitpid = waitpid(lastPid(first), &status, 0);
                    killPid(first);
                    if (!WIFEXITED(status)) {
                        if (result_of_waitpid == -1) perror("Error of waitpid");
                        else {
                            if (!WIFEXITED(status)) {
                                perror("Child process hasn't finished properly\n");
                            } else {
                                printf("Child process has returned %d\n", WEXITSTATUS(status));
                            }
                        }   
                    }
                }
            }
            if (*pid == 0){
                if (close(fd[0]) == -1){
                    perror("of closing pipe");
                }
                ssize_t reswrite = write(fd[1], &res, sizeof(res));
                if (reswrite!=sizeof(res)){
                    perror("of writing pipe");
                }
                if (close(fd[1]) == -1){
                    perror("of closing pipe");
                }
            } else if (first->id == -2) {
                if (close(fd[1]) == -1){
                    perror("of closing pipe");
                }
                ssize_t resread=1;
                for(resread=read(fd[0], &res2, sizeof(res2));resread > 0;
                    resread=read(fd[0], &res2, sizeof(res2))){
                        printf("strcmp %s , %s\n", res2.name, res.name);
                    if (strlen(res2.name) >= strlen(res.name)) {
                        strcpy(res.name, res2.name);
                        printf("res = %s\n", res.name);
                    } 
                }
                if (close(fd[0])==-1){
                    perror("of closing pipe");
                }
                if (resread<0){
                    perror("of read pipe");
                }
                *pid=-2;
            }
        } else perror("error of malloc");
        if (closedir(direct) != 0) perror("error close dir");
        free(first); 
    }
    return res;
}

int main() {
    char dir[1024] = "up88/";
    list * first = 0;
    pid_t pid = 0;
    max final = openDir(dir, &pid);
    if (pid < 0) printf("max len file -> %s\n", final.name);
    return 0;
}
