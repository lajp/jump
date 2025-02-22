//
// Created by toke019 on 8.10.2021.
//

#include <stdio.h>
#include <string.h>

#include "../include/extern.h"
#include "../include/utils.h"

void
usage()
{
    fprintf(stderr, "Use alias: \"./jmp [alias]\". To use it with cd, use command substitution\n");
    fprintf(stderr, "For full help menu, use the -h flag.\n");
}

void
verbose_usage()
{
    printf("Use alias:\t ./jmp <alias> \n");
    printf("New alias:\t jmp <alias> <full path> \n");
    printf("Remove alias:\t jmp -d <alias> \n");
    printf("Reset database:\t jmp -r\n\n");
    printf("Optional flags:\n");
    printf("\t-h: Show help menu\n");
    printf("\t-d: Delete alias. Alias argument required.\n");
    printf("\t-r: Reset database. This flag must be used alone, and asks for confirmation before continuing.\n");
}

void
print_critical(char *message)
{
    fprintf(stderr, "(CRITICAL) %s\n", message);
}

void
print_err(char *message)
{
    fprintf(stderr, "(ERROR) %s\n", message);
}

int raw_confirm(char *message) {
    printf("%s\ntype \"YES\" to confirm: ", message);

    char input[STDIN_MAX_LENGTH];
    fgets(input, STDIN_MAX_LENGTH - 1, stdin);
    input[strcspn(input, "\r\n")] = 0;

    return strncmp(input, "YES", sizeof(input)) == 0;
}

int confirm(char *message) {
    printf("%s\n(y/N): ", message);

    char input[STDIN_MAX_LENGTH];
    fgets(input, STDIN_MAX_LENGTH - 1, stdin);
    input[strcspn(input, "\r\n\t")] = 0;

    return strncmp(input, "y", sizeof(input)) == 0;
}