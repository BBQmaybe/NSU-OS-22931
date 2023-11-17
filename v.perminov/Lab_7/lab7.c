#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/stat.h>

int time_out;

typedef struct {
    off_t offset;
    off_t length;
} Line;

typedef struct {
    Line *array;
    int length;
    int capacity;
} Array;

void initArray(Array *a) {
    a->array = malloc(sizeof(Line));
    a->length = 0;
    a->capacity = 1;
}

void insertArray(Array *a, Line element) {
    if (a->length == a->capacity) {
        a->capacity *= 2;
        a->array = realloc(a->array, a->capacity * sizeof(Line));
    }

    a->array[a->length++] = element;
}

void freeArray(Array *a) {
    free(a->array);
    a->array = NULL;
    a->length = a->capacity = 0;
}

void printLine(Line line, const char *mapped) {
    for (int i = 0; i < line.length; i++) {
        printf("%c", mapped[line.offset + i]);
    }
    printf("\n\n");
}

void timer(int signum) {
    if (signum == SIGALRM) {
        time_out = 1;
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) { return 1; }
    char *path = argv[1];

    Array table;
    initArray(&table);

    int fd = open(path, O_RDONLY);
    if (fd == -1) { return 1; }

    struct stat fileInfo;
    if (fstat(fd, &fileInfo) == -1) { return 1; }
    size_t size = fileInfo.st_size;

    const char *mapped = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) { return 1; }

    off_t lineOffset = 0;
    off_t lineLength = 0;

    for (int i = 0; i < size; i++) {
        char c = mapped[i];
        if (c == '\n') {
            Line current = {lineOffset, lineLength};
            insertArray(&table, current);

            lineOffset += lineLength + 1;
            lineLength = 0;
        } else {
            lineLength++;
        }
    }

    if (lineLength > 0) {
        Line current = {lineOffset, lineLength};
        insertArray(&table, current);
    }

    int num;
    signal(SIGALRM, timer);
    while (1) {
        printf("Enter the line number(0 to leave): ");
        alarm(5);
        time_out = 0;
        scanf("%d", &num);

        if (time_out) {
            printf("...\n");
            for (int i = 0; i < table.length; i++) {
                printf("%d: ", i + 1);
                printLine(table.array[i], mapped);
            }
            break;
        }

        if (num == 0) { break; }
        if (table.length < num) {
            printf("The file contains only %d line(s).\n", table.length);
            continue;
        }

        Line line = table.array[num - 1];
        printLine(line, mapped);
    }

    munmap((void *) mapped, size);
    freeArray(&table);

    return 0;
}

