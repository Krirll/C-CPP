#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <malloc.h>
typedef struct listPid {
    pid_t id;
    struct listPid * next;
}list;

int cleanList(list * first);
int addPid(list * first, pid_t pid);
int killPid(list * first);
pid_t lastPid(list * first);
char _itoa(int count, char * str);
int renaming(struct dirent * item, int count, char current[], char new[], char buffer[]);
int newDir(char buffer[], pid_t * pid);

int addPid(list * first, pid_t pid) {
    int res = 0;
    while (first->next != 0) first=first->next;
	first->next = (list*)malloc(sizeof(list));
	if (first->next != 0){
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
    printf("%s",(res == 0) ? "clean success\n" : "error while clean\n");
    first->id = 0;
    return res;
}

pid_t lastPid(list * first) {
    while (first->next != 0) first = first->next;
    return first->id;
}

char _itoa(int count, char * str) {
    for (int i = count; i >= 10; i /= 10) str++; //go to end of str for record
    for (;count > 0;) {
        *str = count % 10 + '0';
        str--;
        count /= 10;
    }
    return *str;
}

int renaming(struct dirent * item, int count, char current[], char new[], char buffer[]) { //функция для осуществления переименовывания
    int res = 0;
    char charCount[255] = "";
    char * str = charCount;
    _itoa(count, str); //count from 'int' to 'char'
    strcat(new, str); //record count to new name of file
    strcat(new, item->d_name); //record old name to new name
    strcat(current,item->d_name); //record old name in current name
    res = rename(current,new); //renaming old name to new name
    if (res != 0) { //check of renaming
        printf("error of renaming:> %s -> %s\n", current, new);
        res = -1;
    }
    memset(current, 0, sizeof(&current)); //clear strings
    memset(new, 0, sizeof(&new));
    memset(str, 0, 255);
    strcpy(current, buffer); //record the way for next renaming
    strcpy(new, buffer);
    return ++count;
}

int newDir(char buffer[], pid_t * pid) {
    int res = 0;
    int countFile = 1;
    DIR * direct;
    char name[1024] = "up5/";
    list * first = malloc(sizeof(list));
    if (first != 0) {
        first->id = -2;
        first->next = NULL;
        if ((direct = opendir (name)) != NULL) { //check of openning directory
            struct dirent * item = NULL;
            do {
                errno = 0; //for checking error in programm
                if ((item = readdir(direct)) != NULL) {
                    if (item->d_type == 8) { //check of regular file
                        char currentName[1024] = "", newName[1024] = "";
                        strcpy(currentName, name);
                        strcpy(newName, name);
                        printf("current name -> %s\n", currentName);
                        countFile = renaming(item, countFile, currentName, newName, buffer);
                    } else if (item->d_type == 4) {
                        if ((strcmp(item->d_name, ".") != 0) && (strcmp(item->d_name, "..") != 0)) {
                            *pid = fork();
                            if (addPid(first, *pid) == -1) {
                                perror("erorr while add pid");
                                res = -1;
                            }
                            if (lastPid(first) == 0) {
                                countFile = 1;
                                strcat(name, item->d_name);
                                strcat(name, "/");
                                res = closedir(direct);
                                if (res != 0) {
                                    res = -1;
                                    perror("Error of closing directory");
                                }
                                if ((direct = opendir(name)) == NULL) perror("error of opening new dir");
                                cleanList(first);
                                printf("open new directory = %s\n",name);
                            } else if (lastPid(first) == -1) {
                                perror("error of create process");
                                res = -1;
                            }
                        }
                    }
                } else if (item == NULL && errno != 0) { //cherk of error in working
                    perror("error while reading directory:> ");
                    res = -1;
                }  
            } while (item != NULL && res != -1);
        } else {
            perror ("Error of opening directory"); //check of opening directory
            res = -1;
        }
        res = closedir(direct); //check of closing directory
        if (res != 0) {
            res = -1;
            perror("Error of closing directory");
        }
        //Waiting here
        if (lastPid(first) != 0) {
            int status;
            //Здесь надо дождаться завершения дочернего процесса
            while (lastPid(first) != 0 && lastPid(first) != -2) {
                pid_t result_of_waitpid = waitpid(lastPid(first), &status, 0);
                killPid(first);
                if (!WIFEXITED(status)) {
                    if (result_of_waitpid == -1) perror("Error of waitpid");
                    else {
                        //waitpid может произойти успешно, но при этом дочерний процесс может завершиться неуспешно
                        if (!WIFEXITED(status)) {
                            perror("Child process hasn't finished properly\n");
                        } else {
                            printf("Child process has returned %d\n", WEXITSTATUS(status));
                        }
                    }   
                }
            }
        }
    } else {
        perror("error malloc");
        res = -1;
    }
    free(first);
    return (res == 0)? 0 : 1;
}

int main() {
    char buffer[1024] = "up5/";
    pid_t pid = 0;
    int res = newDir(buffer, &pid);
    if (pid < 0){
		printf("%d errors.\n", res);
	}
    return res;
}
