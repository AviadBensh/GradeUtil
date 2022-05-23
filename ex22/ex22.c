//Aviad Benshoshan 318528874 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>

typedef struct GRADE{
    char studentName[50];
    char grade[4];
    char description[20];
}GRADE;

void close_error();
void open_error();
int getNumOfLinesInConfig(int configFD);
void initPaths(int configFD, int configLines, char *Dir, char *input, char *expOutput);
int validatePaths(const char *dirPath, const char *inputPath, const char *outputPath);
int isDir(const char *path);
int isFile(const char *path);
void compile(char *Cfile, GRADE *grade, int inputFD, int errorFD);
void runStudentsFiles(int errorFD, int configFD, int inputFD, int resultsFD, char *StudentsDirPath, char *expectedOutputFile);
void openSingleStudentDir(char *currStudentDir, int errorFD, int configFD, int inputFD, int resultsFD, DIR *dfd,
                          char *expectedOutputFile, GRADE *grade);
void runExecFile(char *StudentDir, int inputFD, int errorFD);
void compareOutputs(char *StudentDir, int inputFD, char *expected, GRADE *grade);
void deleteFiles(char *currStudentDir);


int main(int argc, char **argv) {

    // config file paths
    char StudentsDirPath[151] = " ";
    char InputFilePath[151] = " ";
    char ExpOutputFilePath[151] = " ";
    int configLines;

    // standard error output to errors.txt
    int errorFD = open("errors.txt", O_CREAT | O_WRONLY, 0666 );
    if (errorFD < 0) {
        open_error();
    }
    dup2(errorFD, STDERR_FILENO);

    // file descriptor for configuration file
    int configFD = open(argv[1], O_RDONLY);
    if (configFD < 0) {
        close(errorFD);
        open_error();
    }

    //parse configuration file to paths by lines
    configLines = getNumOfLinesInConfig(configFD);
    lseek(configFD, 0, SEEK_SET); // return fd to start
    initPaths(configFD, configLines, StudentsDirPath, InputFilePath, ExpOutputFilePath);
    if (!validatePaths(StudentsDirPath, InputFilePath, ExpOutputFilePath)) {
        close(errorFD);
        close(configFD);
        exit(-1);
    }
    int inputFD = open(InputFilePath, O_RDONLY, 0111);
    if (inputFD == -1) {
        close(errorFD);
        close(configFD);
        open_error();
    }

    int resultsFD = open("results.csv",  O_WRONLY | O_CREAT | O_APPEND, 0760);
    if (resultsFD == -1) {
        close(errorFD);
        close(configFD);
        close(inputFD);
        open_error();
    }
    runStudentsFiles(errorFD, configFD, inputFD, resultsFD, StudentsDirPath, ExpOutputFilePath);
    if (close(resultsFD) == -1) {
        close_error();
    }
    if (close(inputFD) == -1) {
        close_error();
    }
    if (close(configFD) == -1) {
        close_error();
    }
    if (close(errorFD) == -1) {
        close_error();
    }
}

void open_error(){
    write(1, "error in: open\n", 15);
    exit(-1);
}
void close_error() {
    write(1, "error in: close\n", 16);
    exit(-1);
}
void chdir_error() {
    write(1, "error in: chdir\n", 15);
    exit(-1);
}

int getNumOfLinesInConfig(int configFD) {
    char c;
    int counter = 0;
    ssize_t x = 1;
    while (x != 0) {
        x = read(configFD, &c, 1);
        if (c == '\n') {
            ++counter;
        }
    }
    return (counter - 1);  // subtracting last '\n' - EOF
}

void initPaths(int configFD, int configLines, char *Dir, char *input, char *expOutput) {
    int line;
    for (line = 0; line < configLines; ++line) {
        int i = 0;
        char c = ' ';
        while (c != '\n') {
            int read_check = read(configFD, &c, 1);
            if (read_check == -1) {
                write(1, "error in: read\n", 15);
                exit(-1);
            }
            if (c == '\n') {
                continue;
            }
            switch (line) {
                case 0:
                    Dir[i] = c;
                    break;
                case 1:
                    input[i] = c;
                    break;
                case 2:
                    expOutput[i] = c;
                    break;
                default:
                    break;
            }
            ++i;
        }
    }
}

int validatePaths(const char *dirPath, const char *inputPath, const char *outputPath) {
    if (!isDir(dirPath)) {
        write(1,"Not a valid directory\n", 22);
        return 0;
    }
    if (!isFile(inputPath)) {
        write(1,"Input file not exist\n", 21);
        return 0;
    }
    if (!isFile(outputPath)) {
        write(1,"Output file not exist\n", 22);
        return 0;
    }
    return 1;
}

int isDir(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    int a = S_ISDIR(path_stat.st_mode);
    return a;
}

int isFile(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

void compile(char *Cfile, GRADE *grade, int inputFD, int errorFD) {
    char *gccArgs[] = {"gcc", Cfile, NULL};
    pid_t pid = fork();
    int status;

    // child process
    if (pid == 0) {
        execvp("gcc", gccArgs);
    } else if (pid < 0) {
        write(1, "error in: exec\n", 15);
        exit(-1);
    }
        // parent process
    else if (pid > 0) {
        wait(&status);
        char savePlace[151] = " ";
        getcwd(savePlace, 151);
        chdir("..");
        char currDir[151] = " ";
        getcwd(currDir, 151);
        struct dirent *sdit;
        DIR *sdfd2 = opendir(currDir);
        if (sdfd2 == NULL) {
            close(inputFD);
            close(errorFD);
            open_error();
        }
        int noExecFile = 1;
        while ((sdit = readdir(sdfd2)) != NULL) {
            if (0 == strcmp(sdit->d_name, "a.out")) {
                noExecFile = 0;
            }
        }
        if (noExecFile) {
            strcpy(grade->grade, "10");
            strcat(grade->grade, "\0");
            strcpy(grade->description, "COMPILATION_ERROR");
            strcat(grade->description, "\0");
        }
        chdir(savePlace);
    }
}

void runStudentsFiles(int errorFD, int configFD, int inputFD, int resultsFD, char *StudentsDirPath, char *expectedOutputFile) {
    // directory descriptor for students directory
    struct dirent *DirIter;
    DIR *DirFD;
    DirFD = opendir(StudentsDirPath);
    if (DirFD == NULL)
    {
        close(errorFD);
        close(configFD);
        close(inputFD);
        close(resultsFD);
        open_error();
    }
    while ((DirIter = readdir(DirFD)) != NULL ) {
        char singleStudentDir[151] = " ";
        char currDir[151] = " ";
        strcpy(singleStudentDir, StudentsDirPath);
        strcat(singleStudentDir, "/");
        strcat(singleStudentDir, DirIter->d_name);
        if (DirIter->d_name[0] != '.' && isDir(singleStudentDir)) {
            int chdir_check;
            chdir_check = chdir(singleStudentDir);
            if (chdir_check == -1) {
                chdir_error();
                continue;
            }
            char *cwd_check = getcwd(currDir, 151);
            if (cwd_check == NULL) {
                perror("error in: getcwd\n");
                continue;
            }
            GRADE *grade = (GRADE*) malloc(sizeof (GRADE));
            strcpy(grade->studentName, DirIter->d_name);
            strcat(grade->studentName, "\0");
            openSingleStudentDir(currDir, errorFD, configFD, inputFD, resultsFD, DirFD, expectedOutputFile, grade);
            free(grade);
            chdir_check = chdir("..");
            if (chdir_check == -1) {
                close(errorFD);
                close(configFD);
                close(inputFD);
                close(resultsFD);
                chdir_error();
            }
            chdir_check = chdir("..");
            if (chdir_check == -1) {
                close(errorFD);
                close(configFD);
                close(inputFD);
                close(resultsFD);
                chdir_error();
            }
        }
    }
    if (closedir(DirFD) == -1) {
        close_error();
    }
}

void openSingleStudentDir(char *currStudentDir, int errorFD, int configFD, int inputFD, int resultsFD, DIR *dfd,
                          char *expectedOutputFile, GRADE *grade) {
    // single student directory descriptor
    struct dirent *subDirIter;
    DIR *subDirFD = opendir(currStudentDir);
    if (subDirFD == NULL)
    {
        close(errorFD);
        close(configFD);
        close(inputFD);
        close(resultsFD);
        closedir(dfd);
        open_error();
    }
    int C_File = 0;
    while ((subDirIter = readdir(subDirFD)) != NULL) {
        char InnerFile[151] = " ";
        strcpy(InnerFile, currStudentDir);
        strcat(InnerFile, "/");
        strcat(InnerFile, subDirIter->d_name);
        if (isFile(InnerFile)) {
            int i = 0;
            while (subDirIter->d_name[i] != '\0') {
                if (subDirIter->d_name[i] == '.' && subDirIter->d_name[i + 1] == 'c' && subDirIter->d_name[i + 2] == '\0') {
                    C_File = 1;
                    char fullPath[151] = " ";
                    char temp[151] = " ";
                    strcpy(temp, currStudentDir);
                    strcat(temp, "/");
                    strcat(temp, subDirIter->d_name);
                    realpath(temp, fullPath);
                    compile(fullPath, grade, inputFD, errorFD);
                    runExecFile(currStudentDir, inputFD, errorFD);
                    compareOutputs(currStudentDir, inputFD, expectedOutputFile, grade);
                    deleteFiles(currStudentDir);
                    temp[0] = '\0';
                    fullPath[0] = '\0';
                    break;
                }
                ++i;
            }
        }
    }
    if (!C_File) {
        strcpy(grade->grade, "0");
        strcat(grade->grade, "\0");
        strcpy(grade->description, "NO_C_FILE");
        strcat(grade->description, "\0");
    }
    write(resultsFD, grade->studentName, strlen(grade->studentName));
    write(resultsFD, ",", 1);
    write(resultsFD,  grade->grade, strlen(grade->grade));
    write(resultsFD, ",", 1);
    write(resultsFD, grade->description, strlen(grade->description));
    write(resultsFD, "\n", 1);
}

void runExecFile(char *StudentDir, int inputFD, int errorFD) {

    // single student directory descriptor
    struct dirent *sdit;
    DIR *sdfd2 = opendir(StudentDir);
    if (sdfd2 == NULL)
    {
        close(inputFD);
        open_error();
    }
    while ((sdit = readdir(sdfd2)) != NULL) {
        char InnerFile[151] = " ";
        strcpy(InnerFile, StudentDir);
        strcat(InnerFile, "/");
        strcat(InnerFile, sdit->d_name);
        if (isFile(InnerFile)) {
            if (0 == strcmp(sdit->d_name, "a.out")) {
                int chdir_check;
                char fullPath[151] = " ";
                char temp[151] = " ";
                char execCommand[153] = "./";
                char currentDir[151] = " ";
                char *cwd_check = getcwd(currentDir, 151);
                if (cwd_check == NULL) {
                    perror("error in: getcwd\n");
                    continue;
                }
                strcpy(temp, StudentDir);
                strcat(temp, "/");
                chdir_check = chdir(temp);
                if (chdir_check == -1) {
                    chdir_error();
                    continue;
                }
                strcat(execCommand, sdit->d_name);
                char *gccArgs[] = {execCommand, NULL};

                pid_t pid = fork();
                int status;
                int stuOutput;
                // child process
                if (pid == 0) {
                    stuOutput = open("my_output.txt", O_RDWR | O_CREAT, 0666);
                    if (stuOutput == -1) {
                        open_error();
                    }
                    dup2(inputFD, 0);
                    dup2(stuOutput, 1);
                    dup2(errorFD, 2);
                    execvp(execCommand, gccArgs);
                }
                // fork returned error - did not succeed
                else if (pid < 0) {
                    perror("fork failed\n");
                }

                // parent process
                else if (pid > 0) {
                    wait(&status);
                    if (status < 0) {
                        perror("compile failed\n");
                    }
                    lseek(inputFD, 0, SEEK_SET);
                    chdir_check = chdir(currentDir);
                    if (chdir_check == -1) {
                        close(inputFD);
                        chdir_error();
                        exit(-1);
                    }
                }

                temp[0] = '\0';
                fullPath[0] = '\0';
                break;
            }
        }
    }
}

void compareOutputs(char *StudentDir, int inputFD, char *expected, GRADE *grade) {
    char curr[151] = " ";
    char *cwd_check = getcwd(curr, 151);
    if (cwd_check == NULL) {
        perror("error in: getcwd\n");
        strcpy(grade->grade, "0");
        strcat(grade->grade, "\0");
        strcpy(grade->description, "incomparable");
        strcat(grade->description, "\0");
    }
    // single student directory descriptor
    struct dirent *sdit;
    DIR *sdfd2 = opendir(StudentDir);
    if (sdfd2 == NULL)
    {
        close(inputFD);
        open_error();
    }
    while ((sdit = readdir(sdfd2)) != NULL) {
        char InnerFile[151] = " ";
        strcpy(InnerFile, StudentDir);
        strcat(InnerFile, "/");
        strcat(InnerFile, sdit->d_name);
        if (isFile(InnerFile)) {
            if (0 == strcmp(sdit->d_name, "my_output.txt")) {
                int chdir_check;
                chdir_check = chdir("..");
                if (chdir_check == -1) {
                    close(inputFD);
                    chdir_error();
                }
                chdir_check = chdir("..");
                if (chdir_check == -1) {
                    close(inputFD);
                    chdir_error();
                }
                cwd_check = getcwd(curr, 151);
                if (cwd_check == NULL) {
                    perror("error in: getcwd\n");
                    strcpy(grade->grade, "0");
                    strcat(grade->grade, "\0");
                    strcpy(grade->description, "incomparable");
                    strcat(grade->description, "\0");
                }
                char stuOutput[151] = " ";
                strcpy(stuOutput, StudentDir);
                strcat(stuOutput, "/");
                strcat(stuOutput, sdit->d_name);
                char *args[] = {"./comp.out", expected, stuOutput, NULL};
                pid_t pid;
                int status;
                pid = fork();
                if (pid == 0) {
                    execvp("./comp.out", args);
                }
                    // fork returned error - did not succeed
                else if (pid < 0) {
                    perror("fork failed\n");
                }
                    // parent process
                else if (pid > 0) {
                    wait(&status);
                    if (WIFEXITED(status)) {
                        int return_value = WEXITSTATUS(status); // Extract return value from status
                        if (return_value < 0) {
                            strcpy(grade->grade, "10");
                            strcat(grade->grade, "\0");
                            strcpy(grade->description,"COMPILATION_ERROR");
                            strcat(grade->description, "\0");
                        } else if (return_value == 1) {
                            strcpy(grade->grade, "100");
                            strcat(grade->grade, "\0");
                            strcpy(grade->description,"EXCELLENT");
                            strcat(grade->description, "\0");
                        } else if (return_value == 2) {
                            strcpy(grade->grade, "50");
                            strcat(grade->grade, "\0");
                            strcpy(grade->description,"WRONG");
                            strcat(grade->description, "\0");
                        } else if (return_value == 3) {
                            strcpy(grade->grade, "75");
                            strcat(grade->grade, "\0");
                            strcpy(grade->description,"SIMILAR");
                            strcat(grade->description, "\0");
                        }
                    }
                    chdir_check = chdir(StudentDir);
                    if (chdir_check == -1) {
                        close(inputFD);
                        chdir_error();
                        exit(-1);
                    }
                }
            }
        }
    }
}

void deleteFiles(char *StudentDir) {
    char curr[151] = " ";
    char *cwd_check = getcwd(curr, 151);
    if (cwd_check == NULL) {
        perror("error in: getcwd\n");
        return;
    }
    // single student directory descriptor
    struct dirent *sdit;
    DIR *sdfd2 = opendir(StudentDir);
    if (sdfd2 == NULL)
    {
        open_error();
    }
    while ((sdit = readdir(sdfd2)) != NULL) {
        char InnerFile[151] = " ";
        strcpy(InnerFile, StudentDir);
        strcat(InnerFile, "/");
        strcat(InnerFile, sdit->d_name);
        if (isFile(InnerFile)) {
            if (0 == strcmp(sdit->d_name, "my_output.txt")) {
                char *gccArgs[] = {"rm", "my_output.txt", NULL};
                pid_t pid;
                int status;
                pid = fork();
                if (pid < 0) {
                    write(1, "error in: fork\n", 15);
                }
                else if (pid == 0) {
                    execvp("rm", gccArgs);
                }
                else if (pid > 0) {
                    wait(&status);
                }
            }
            if (0 == strcmp(sdit->d_name, "a.out")) {
                char *gccArgs[] = {"rm", "a.out", NULL};
                pid_t pid;
                int status;
                pid = fork();
                if (pid < 0) {
                    write(1, "error in: fork\n", 15);
                }
                else if (pid == 0) {
                    execvp("rm", gccArgs);
                }
                else if (pid > 0) {
                    wait(&status);
                }
            }
        }
    }
}
