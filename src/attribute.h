#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include "includes.h"




std::string folderAttributeConfigurator(FatFile83 piece)
{
    std::string attribTmp("drwx------ 1 root root ");

    uint16_t modifiedDate = piece.modifiedDate;
    uint16_t month = (modifiedDate & 0x01E0) >> 5;
    uint16_t day = (modifiedDate & 0x001F);

    uint16_t modifiedTime = piece.modifiedTime;
    uint16_t hour = (modifiedTime & 0xF800) >> 11;
    uint16_t min = (modifiedTime & 0x07E0) >> 5;

    attribTmp += std::to_string(0) + 
    " " + months[month - 1] + " " +  std::to_string((int)day) + " "
    + std::to_string((int)hour) + ":";

    if(min < 0x000A) attribTmp += ("0" + std::to_string((int) min)); 
    else attribTmp += std::to_string((int) min);

    return attribTmp;
}

std::string fileAttributeConfigurator(FatFile83 piece)
{

    std::string attribTmp("-rwx------ 1 root root ");
    uint16_t modifiedDate = piece.modifiedDate;

    uint16_t month = (modifiedDate & 0x01E0) >> 5;
    uint16_t day = (modifiedDate & 0x001F);

    uint32_t fileSize = piece.fileSize;

    uint16_t modifiedTime = piece.modifiedTime;
    uint16_t hour = (modifiedTime & 0xF800) >> 11;
    uint16_t min = (modifiedTime & 0x07E0) >> 5 ;

    attribTmp = attribTmp + std::to_string((int)fileSize) + 
    " " + months[month - 1] + " " +  std::to_string((int)day) + " "
    + std::to_string((int)hour) + ":";


    if(min < 0x000A) attribTmp += ("0" + std::to_string((int) min));
    else attribTmp += std::to_string((int) min);

    return attribTmp;

}

void pushToString(std::string & to, FatFileLFN from)
{
    for(int i = 0; i < 5; i++){
        if(from.name1[i] == '\0') return;
        uint16_t curr = from.name1[i];

        if(curr > 255 || !curr) continue;
        to.push_back((char) curr);
    }

    for(int i = 0; i < 6; i++){
        if(from.name2 [i]== '\0') return;
        uint16_t curr = from.name2[i];

        if(curr > 255 || !curr) continue;
        to.push_back((char) curr);
    }

    for(int i = 0; i < 2; i++){
        if(from.name2 [i]== '\0') return;
        uint16_t curr = from.name3[i];

        if(curr > 255 || !curr) continue;
        to.push_back((char) curr);
    }
    return;
}




#endif