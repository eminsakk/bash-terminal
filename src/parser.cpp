#include "parser.h"

void parse(parsed_input* inp, char *line) {
    char *tmp;
    unsigned long size;

    size = strlen(line);

    if ( line[size-1] == '\n' )
        line[size-1] = '\0';

    tmp = strtok(line, " ");

    inp->arg1 = inp->arg2 = NULL;
    if ( !strcmp(tmp, "cd") ) {
        inp->type = CD;
    }
    else if ( !strcmp(tmp, "ls") ) {
        inp->type = LS;
    }
    else if ( !strcmp(tmp, "mkdir") ) {
        inp->type = MKDIR;
    }
    else if ( !strcmp(tmp, "touch") ) {
        inp->type = TOUCH;
    }
    else if ( !strcmp(tmp, "mv") ) {
        inp->type = MV;
    }
    else if ( !strcmp(tmp, "cat") ) {
        inp->type = CAT;
    }
    else if( !strcmp(tmp, "quit") ) {
        inp->type = QUIT;
    }

    tmp = strtok(NULL, " ");

    if( tmp )
    {
        size = strlen(tmp);

        inp->arg1 = (char*) calloc(size+1, sizeof(char));
        strcpy(inp->arg1, tmp);
    }
    

    tmp = strtok(NULL, " ");

    if ( tmp ) {
        size = strlen(tmp);

        inp->arg2 = (char*) calloc(size+1, sizeof(char));
        strcpy(inp->arg2, tmp);
    }
}

void clean_input(parsed_input* inp) {
    free(inp->arg1);
    if ( inp->arg2 )
        free(inp->arg2);
}


// Parse input for given delimeter.
std::vector<std::string> parse_path(const char * path,char delim,int len)
{
    
    std::vector<std::string> res;
    std::string tmp = "";
    int cnt = 0;
    for(int i = 0; i < len;i++){
        if(path[i] == delim){
            if(tmp != "")
                res.push_back(tmp);
            tmp = "";    
        }
        else{
            tmp.push_back(path[i]);
        }
    }
    if(tmp != "")   res.push_back(tmp);
    return res;
}

// Takes prompt objects and creates absolute path vector.
std::vector<std::string> configurePath(std::vector<std::string> & path,std::vector<std::string> & promptPath)
{
    std::vector<std::string> configuredPath;
    configuredPath.insert(configuredPath.end(), promptPath.begin(),promptPath.end());
    promptPath.clear();

    int size = path.size();
    for(int i = 0; i < size; i++)
    {
        if(path[i] == ".." && configuredPath.size() != 0)
        {
            configuredPath.pop_back();
        }
        else
        {
            configuredPath.push_back(path[i]);
        }
    }
    return configuredPath;

}

// Updates prompt object vector from prompt string.
std::vector<std::string> gatherObjFromPrompt(std::vector<std::string> & obj, std::string prompt)
{
    std::vector<std::string> res;
    std::string x = prompt.substr(1,prompt.size() - 3);
    const char * send = x.c_str();
    res = parse_path(send,'/',x.size());
    return res;
}