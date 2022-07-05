#ifndef PARSER_H
#define PARSER_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <iostream>

typedef enum input_type {
    CD,
    LS,
    MKDIR,
    TOUCH,
    MV,
    CAT,
    QUIT
}input_type;

typedef struct parsed_input {
    input_type type;
    char *arg1;
    char *arg2;
} parsed_input;
/*
 * Parses a single line of input and separates it into arguments
 * It does not accept wrong input and all arguments must be separated with a space
 * It can have a newline or not at the end
 * The string must naturally terminate with '\0'
 * */
void parse(parsed_input* inp, char *line);

/* Free the argument arrays. Use before discarding the arguments, otherwise there will be memory leaks.*/
void clean_input(parsed_input* inp);

std::vector<std::string> parse_path(const char * path,char delim,int len);

std::vector<std::string> configurePath(std::vector<std::string> & path,std::vector<std::string> & promptPath);

std::vector<std::string> gatherObjFromPrompt(std::vector<std::string> & obj, std::string prompt);



#endif