//Aviad Benshoshan  318528874 
#include <stdio.h>
#include <unistd.h>
#include <sys/file.h>
#include <stdlib.h>

int caseDiff(char a, char b);
int isSpace(char a);
int findDiff(char c1, char c2, int fd1, int fd2);

int main(int argc, char **argv) {
    if (argc != 3) {
        perror("wrong number of arguments\n");
        _exit (-1);
    }
    char c1, c2;    // chars in file
    char x1, x2;    // number of bytes read from file
    int ret = 1;    // default return - files are equal
    int fd1 = open(argv[1], O_RDONLY);
    if (fd1 < 0) {
        _exit (-1);
    }
    int fd2 = open(argv[2], O_RDONLY);
    if (fd2 < 0) {
        close(fd1);
        _exit (-1);
    }
    // loop as long as bytes are equal, otherwise - go to findDiff func to determine difference
    while(1) {
        x1 = read(fd1, &c1, 1);
        if (x1 < 0) {
            close(fd1);
            close(fd2);
            _exit (-1);
        }
        x2 = read(fd2, &c2, 1);
        if (x2 < 0) {
            close(fd1);
            close(fd2);
            _exit (-1);
        }
        // no bytes were read, end of file
        if (0 == x1 && 0 == x2) {
            close(fd1);
            close(fd2);
            _exit(1);
        }
        // bytes are equal
        else if (c1 == c2) {
            continue;
        }
        // bytes are different
        else  {
            return findDiff(c1, c2, fd1, fd2);
        }
    }
}

int findDiff(char c1, char c2, int fd1, int fd2) {
    char x1 = 1, x2 = 1;

    // pass all spaces
    while (isSpace(c1) && x1 != 0) {
        x1 = read (fd1, &c1, 1);
    }
    while (isSpace(c2) && x2 != 0) {
        x2 = read (fd2, &c2, 1);
    }

    // if both files ended, difference was in spaces
    if (0 == x1 || 0 == x2) {
        _exit(3);
    }

        // if characters are strictly different, no need to continue
    else if (c1 != c2 && !caseDiff(c1, c2)) {
        close(fd1);
        close(fd2);
        _exit(2);
    }
    while(1) {

        //skip spaces
        do {
            x1 = read(fd1, &c1, 1);
            if (x1 < 0) {
                close(fd1);
                close(fd2);
                _exit (-1);
            }
        }
        while (isSpace(c1) && x1 != 0);

        do {
            x2 = read(fd2, &c2, 1);
            if (x2 < 0) {
                close(fd1);
                close(fd2);
                _exit (-1);
            }
        }
        while (isSpace(c2) && x2 != 0);

        if (c1 != c2 && !caseDiff(c1, c2) && !(isSpace(c1) || isSpace(c2))) {
            close(fd1);
            close(fd2);
            _exit(2);
        }
            // no bytes were read, end of file
        else if (0 == x1 && 0 == x2) {
            close(fd1);
            close(fd2);
            _exit(3);
        }
        else if (x1 != x2) {
            close(fd1);
            close(fd2);
            _exit(2);
        }
    }
}

int caseDiff(char a, char b) {
    if (a - 32 == b || a == b - 32) {
        return 1;
    }
    return 0;
}

int isSpace(char a) {
    if (a == ' ' || a == '\n') {
        return 1;
    }
    return 0;
}


