#include <iostream>
#include <vector>
#include "parser.h"
#include "includes.h"
#include "commands.h"
#include "fat32dependent.h"

int main(int argc, char * argv[])
{
    std::string prompt = "/> ";
    std::vector<std::string> promptObjects;

    char * imageName = (char * ) malloc(sizeof(char) * strlen(argv[1]));
    strcpy(imageName,argv[1]);
    FILE * img = fopen(imageName, "rb");
    bool ok = readBPBsector(img);
    
    // Some initializations.
    uint32_t rootClusterNumber = bootSector.extended.RootCluster;
    uint16_t sectorInBytes = bootSector.BytesPerSector;
    uint8_t numbOfFats = bootSector.NumFATs;
    uint32_t fatInSectors = bootSector.extended.FATSize;
    uint16_t rezSectorCount = bootSector.ReservedSectorCount;

    uint32_t dataRegionOffset = numbOfFats * fatInSectors * sectorInBytes 
                              + rezSectorCount * sectorInBytes;
    uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
    bool close_ok = (fclose(img) == 0);
    
    while(true)
    {   
        std::cout << prompt;
        parsed_input input = parsed_input();
        input.arg1 = NULL;
        input.arg2 = NULL;

        char line[MAX_LEN];
        memset(line,0,sizeof(line));
        fgets(line,MAX_LEN,stdin);

        parse(&input,line);

        if(input.type == QUIT)
        {
            break;
        }
        else if(input.type == CD)
        {
            // If directory exists change prompt. Otherwise no error specified.
            // Program should continue.
            int _size = strlen(input.arg1);
            std::vector<std::string> path = parse_path(input.arg1,'/',_size); 

            // We get actual directory path from the beginning.
            path = configurePath(path,promptObjects);
            bool open_ok = ((img = fopen(imageName, "rb")) != NULL);
            if(cdCommand(dataRegionOffset,img,path) && open_ok)
            {
                prompt.clear();
                prompt.push_back('/');
                for(int i = 0; i < path.size();i++){
                    promptObjects.push_back(path[i]);
                    prompt.append(path[i]);
                    if(i != path.size() - 1) prompt.append("/");
                }
                prompt.append("> ");
            }
            else
                promptObjects = gatherObjFromPrompt(promptObjects,prompt);
            

        }
        else if(input.type == LS)
        {
            bool open_ok = ((img = fopen(imageName, "rb")) != NULL);
            if(input.arg1)
            {
                if(input.arg2)
                {
                    // ls <flag> <path> entered.
                    // print files and directories with permissions... 
                    int _size = strlen(input.arg2);
                    std::vector<std::string> path = parse_path(input.arg2,'/',_size);
                    path = configurePath(path,promptObjects);
                    bool ls_ok = lsCommand(dataRegionOffset,img,path,true);
                    promptObjects = gatherObjFromPrompt(promptObjects,prompt);
                }
                else
                {
                    // ls <flag> or ls <path> entered.
                    if( !strcmp(input.arg1,"-l") )
                    {
                        // ls <flag> entered.
                        // Show current directory files and directories with permissions...
                        std::vector<std::string> path;
                        path.insert(path.end(),promptObjects.begin(),promptObjects.end());
                        bool ls_ok = lsCommand(dataRegionOffset,img,path,true);
                    }
                    else
                    {
                        // ls <path> entered.
                        int _size = strlen(input.arg1);
                        std::vector<std::string> path = parse_path(input.arg1,'/',_size);
                        path = configurePath(path,promptObjects);
                        bool ls_ok = lsCommand(dataRegionOffset,img,path,false);
                        promptObjects = gatherObjFromPrompt(promptObjects,prompt);
                    }
                }
            }
            else
            {
                // Just ls command entered.
                // Print files only.
                std::vector<std::string> path;
                path.insert(path.end(),promptObjects.begin(),promptObjects.end());
                bool ls_ok = lsCommand(dataRegionOffset,img,path,false);
            }
        }
        else if(input.type == MKDIR)
        {
            int _size = strlen(input.arg1);
            std::vector<std::string> path = parse_path(input.arg1, '/',_size);
            path = configurePath(path,promptObjects);
            std::string folder_name = path[path.size() - 1];
            path.pop_back();
            bool open_ok = ((img = fopen(imageName, "rb+")) != NULL);
            mkdirCommand(img,dataRegionOffset,path,folder_name);
            promptObjects = gatherObjFromPrompt(promptObjects,prompt);
        }
        else if(input.type == TOUCH)
        {
            int _size = strlen(input.arg1);
            std::vector<std::string> path = parse_path(input.arg1, '/',_size);
            path = configurePath(path,promptObjects);
            std::string file_name = path[path.size() - 1];
            path.pop_back();
            bool open_ok = ((img = fopen(imageName, "rb+")) != NULL);
            touchCommand(img,dataRegionOffset,path,file_name);
            promptObjects = gatherObjFromPrompt(promptObjects,prompt);

        }
        else if(input.type == MV)
        {
            bool open_ok = ((img = fopen(imageName, "rb+")) != NULL);
            int _size = strlen(input.arg1);
            std::vector<std::string> sourcePath = parse_path(input.arg1, '/',_size);
            std::string fileToBeMove= sourcePath[sourcePath.size() - 1];
            sourcePath.pop_back();
            sourcePath = configurePath(sourcePath,promptObjects);
            promptObjects = gatherObjFromPrompt(promptObjects,prompt);
            _size = strlen(input.arg2);
            std::vector<std::string> destinationPath = parse_path(input.arg2,'/',_size);
            if(destinationPath[0] != ".")
            {
                destinationPath = configurePath(destinationPath,promptObjects);
                promptObjects = gatherObjFromPrompt(promptObjects,prompt);
            }
            else
            {
                destinationPath.pop_back();
                destinationPath = configurePath(destinationPath,promptObjects);
                promptObjects = gatherObjFromPrompt(promptObjects,prompt);
            }
            moveCommand(img,dataRegionOffset,fileToBeMove,sourcePath,destinationPath);

        }
        else if(input.type == CAT)
        {
            int _size = strlen(input.arg1);
            std::vector<std::string> path = parse_path(input.arg1,'/',_size);
            std::string file_name = path[path.size() - 1];
            path.pop_back();
            path = configurePath(path,promptObjects);
            bool open_ok = ((img = fopen(imageName, "rb")) != NULL);
            bool cat_ok = catCommand(img,dataRegionOffset,path,file_name);
            promptObjects = gatherObjFromPrompt(promptObjects,prompt);
        }
        clean_input(&input);
        close_ok = (fclose(img) == 0);
    }
    return 0;
}