#ifndef COMMANDS_H
#define COMMANDS_H

#include "includes.h"
#include "fat32dependent.h"


/***
 * There are lots of code repatition of this project.
 * Because I have to complete this assignment in my final week I need to 
 * copy paste same code many times. Sorry :)
 * ***/


// General Finder from beginning. 
bool generalFinder(FILE * img, uint32_t clusterNo, uint32_t dataRegionOffset,std::string toBeFind,
                   uint32_t & fileClusterNo, uint32_t & file_size)
{
    uint32_t currClusterNo = clusterNo;
    uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
    uint32_t clusterOffset = clusterSizeInBytes * (currClusterNo - 2);
    bool fseek_ok = (fseek(img,dataRegionOffset + clusterOffset,SEEK_SET) == 0);
    uint32_t entryCount = 0;

    std::vector<FatFileLFN> LFNs;
    while (true)
    {
        FatFileEntry entry;
        bool read_ok = fread(&entry,sizeof(entry),1,img);
        entryCount++;
        
        if(entry.lfn.attributes == 0x00)
        {
            uint32_t nextCluster = calculateNextChainIdx(img,currClusterNo);

            if(nextCluster >= 0x0FFFFFF8)
            {
                // End Of Chain.
                return false;
            }
            else 
            {
                // There is next cluster.
                currClusterNo = nextCluster;
                entryCount = 0;
                clusterOffset = clusterSizeInBytes * (currClusterNo - 2);
                fseek_ok = (fseek(img,dataRegionOffset + clusterOffset,SEEK_SET) == 0);
                continue;
            }
        }
        else if(entry.lfn.attributes == 0x0F)
        {
            // LFN geldi.
            LFNs.push_back(entry.lfn);
        }
        else if(entry.msdos.attributes == 0x10)
        {
            // Folder
            std::string tmp;

            for(int iter = LFNs.size() - 1; iter >= 0;iter--)
            {
                FatFileLFN currLFN = LFNs[iter];
                pushToString(tmp, currLFN);
            }
            LFNs.clear();
            if(tmp == toBeFind)
            {
                fileClusterNo = (entry.msdos.eaIndex << 16) | entry.msdos.firstCluster;
                return true;
            }
        }
        else if(entry.msdos.attributes == 0x20)
        {
            // File
            std::string tmp;
            for(int iter = LFNs.size() - 1; iter >= 0;iter--)
            {
                FatFileLFN currLFN = LFNs[iter];
                pushToString(tmp, currLFN);
            }
            LFNs.clear();

            if(tmp == toBeFind)
            {
                fileClusterNo = entry.msdos.firstCluster;
                file_size = entry.msdos.fileSize;
                return true;
            }
        }
        if(entryCount == (bootSector.BytesPerSector * bootSector.SectorsPerCluster / 32))
        {
            entryCount = 0;
            uint32_t nextIdx = calculateNextChainIdx(img,currClusterNo);
            if(nextIdx >= 0x0FFFFFF8){
                return false;
            }
            else
            {
                currClusterNo = nextIdx;
                clusterOffset = clusterSizeInBytes * (currClusterNo - 2);
                fseek_ok = (fseek(img,dataRegionOffset + clusterOffset,SEEK_SET));
                continue;
            }
        }
        else
            fseek_ok = (fseek(img,dataRegionOffset + entryCount * 32 + clusterOffset,SEEK_SET) == 0);
    }
    return false;
}


// Same as change directory command (cd) in linux. Path can be absolute or relative.
bool cdCommand(uint32_t dataRegionOffset, FILE * img,std::vector<std::string>& path)
{
    // Eğer directory, current directory de yoksa return true else false
    uint32_t clusterNo = bootSector.extended.RootCluster;
    int path_size = path.size();
    bool seek_ok = (fseek(img,dataRegionOffset,SEEK_SET) == 0);
    for(int i = 0; i < path_size; i++){
        std::string dir_name = path[i];
        uint32_t nextCluster;
        uint32_t x;
        if(generalFinder(img,clusterNo,dataRegionOffset,dir_name,nextCluster,x))
            clusterNo = nextCluster;
        else
            return false;
    }
    return true;
}


// ls command subroutine. We get entries and their attributes from img file 
// and print it. If flag is true then attriutes are also printed (just like ls -l in Linux). 
void lsPrint(std::vector<std::string> entries, std::vector<std::string> attributes,bool flag)
{
    for(int i = 0; i < entries.size();i++)
    {
        if(!flag)
        {
            // ls just
            if(i == entries.size() - 1) 
                std::cout << entries[i] << std::endl;
            else std::cout << entries[i] << " "; 
        }
        else
        {
            // ls with attributes.
                std::cout << attributes[i] << " " << entries[i] << std::endl;
        }
    }
}


// Seeks actual directory entries.
bool lsSubroutine(FILE * img,uint32_t dataRegionOffset, uint32_t cluster_offset_first,uint32_t cluster_first,bool flag)
{
    bool seek_ok = (fseek(img,dataRegionOffset + cluster_offset_first,SEEK_SET) == 0);
    uint32_t cluster_offset = cluster_offset_first;
    uint32_t clusterNo = cluster_first;
    bool loopFlag = false;
    uint32_t entryCount = 0;
    std::vector<std::string> toBePrinted;
    std::vector<std::string> attributes;
    std::vector<FatFileLFN> LFNs;
    while(true)
    {

        FatFileEntry piece;
        bool read_ok = fread(&piece,sizeof(piece),1,img);
        entryCount++;

        if(piece.lfn.attributes == 0){
            uint32_t idx = calculateNextChainIdx(img,clusterNo);
            if(idx >= 0x0FFFFFF8)
            {
                break;
            }
            else
            {
                clusterNo = idx;
                cluster_offset = bootSector.BytesPerSector * bootSector.SectorsPerCluster * (clusterNo - 2);
                seek_ok = (fseek(img,dataRegionOffset + cluster_offset,SEEK_SET));
                entryCount = 0;
                continue;
            }                
        }
        else if(piece.lfn.attributes == 0x0F && piece.lfn.sequence_number != 0xE5)
        {
            // LFN geldi.
            LFNs.push_back(piece.lfn);
        }
        else if(piece.msdos.attributes == 0x10 && piece.msdos.filename[0] != 0xE5)
        {

            std::string tmp;
            for(int iter = LFNs.size() - 1; iter >= 0;iter--)
            {
                FatFileLFN currLFN = LFNs[iter];
                pushToString(tmp, currLFN);
            }
            if(tmp != ""){
                toBePrinted.push_back(tmp);
                attributes.push_back(folderAttributeConfigurator(piece.msdos));    
            } 
            LFNs.clear();    
        }
        else if(piece.msdos.attributes == 0x20 && piece.msdos.filename[0] != 0xE5)
        {

            std::string tmp;
            for(int iter = LFNs.size() - 1; iter >= 0;iter--)
            {
                FatFileLFN currLFN = LFNs[iter];
                pushToString(tmp, currLFN);
            }
            if(tmp != "")
            {
                toBePrinted.push_back(tmp);
                attributes.push_back(fileAttributeConfigurator(piece.msdos));
            } 
            LFNs.clear();
        }

        if(entryCount == (bootSector.BytesPerSector * bootSector.SectorsPerCluster / 32)) // Değişecek
        {
            // EntryCount 32 olursa direk next cluster hesapla
            entryCount = 0;
            uint32_t nextIdx = calculateNextChainIdx(img,clusterNo);
            if(nextIdx >= 0x0FFFFFF8){
                break;
            }
            else
            {
                clusterNo = nextIdx;
                //Seek olayları.
                cluster_offset = bootSector.BytesPerSector * bootSector.SectorsPerCluster * (clusterNo - 2);
                seek_ok = (fseek(img,dataRegionOffset + cluster_offset,SEEK_SET));
                continue;
            }

        }
        else seek_ok = (fseek(img,dataRegionOffset + entryCount * 32 + cluster_offset,SEEK_SET) == 0);
    }
    lsPrint(toBePrinted,attributes,flag);
    return true;
}


// Seeks and finds 
bool lsCommand(uint32_t dataRegionOffset, FILE * img, std::vector<std::string>& path, bool flag)
{
    int clusterNo = bootSector.extended.RootCluster;
    bool loopFlag = false;
    uint32_t clusterOffset = bootSector.BytesPerSector * bootSector.SectorsPerCluster * (clusterNo - 2);
    int path_size = path.size();
    bool seek_ok = (fseek(img,dataRegionOffset,SEEK_SET) == 0);

    for(int i = 0; i < path_size; i++){

        std::string dir_name = path[i];
        FatFileEntry piece;
        std::vector<FatFileLFN> LFNs;
        uint32_t entryCount = 0;
        uint32_t nextCluster;
        uint32_t x;
        if(generalFinder(img,clusterNo,dataRegionOffset,dir_name,nextCluster,x))
        {
            clusterNo = nextCluster;
            clusterOffset = bootSector.BytesPerSector * bootSector.SectorsPerCluster * (clusterNo - 2);
        }
        else
        {
            return false;
        }
    }
    return lsSubroutine(img,dataRegionOffset,clusterOffset,clusterNo,flag);
}


// Read boot sector from fat32 formatted img.
bool readBPBsector(FILE * img)
{
    return fread(&bootSector,sizeof(bootSector),1,img) > 0;
}


// Subroutine for cat command. Prints file to the screen.
void printFile(uint32_t clusterNumber, FILE * img,uint32_t dataRegionOffset,uint32_t file_size)
{
    if(file_size == 0) return;

    uint32_t fileClusterChainHolder = clusterNumber;
    uint32_t clusterSizeInBytes = bootSector.SectorsPerCluster * bootSector.BytesPerSector;
    bool seek_ok = (fseek(img,dataRegionOffset + (fileClusterChainHolder - 2) * clusterSizeInBytes,SEEK_SET) == 0);
    uint32_t readedByteCount = 0;
    uint32_t byteIterator = 0;
    char * text = (char *) malloc(sizeof(char) * (int) file_size + 1);
    while(readedByteCount <= file_size)
    {
        uint8_t tmp;   
        bool read_ok = fread(&tmp,sizeof(tmp),1,img);
        if(tmp <= 127)
        {
            text[readedByteCount] = (char)tmp;
            readedByteCount++;
        }
        
        if(byteIterator < clusterSizeInBytes)
        {
            byteIterator++;
            bool seek_ok = (fseek(img,dataRegionOffset + (fileClusterChainHolder - 2) * clusterSizeInBytes + byteIterator,SEEK_SET) == 0);
        }
        else
        {


            byteIterator = 0;
            fileClusterChainHolder = calculateNextChainIdx(img,fileClusterChainHolder);
        }

    }
    text[file_size] = '\0';
    std::cout << text << std::endl;
}


// Seeks fileName and prints it using printFile function.
bool catCommand(FILE * img, uint32_t dataRegionOffset,std::vector<std::string>& path, std::string textToRead)
{
    int clusterNo = bootSector.extended.RootCluster;
    bool loopFlag = false;
    uint32_t clusterOffset = 0;
    int path_size = path.size();
    uint32_t nextCluster;
    uint32_t x;
    bool seek_ok = (fseek(img,dataRegionOffset,SEEK_SET) == 0);
    for(int i = 0; i < path_size; i++){

        std::string dir_name = path[i];
        if(generalFinder(img,clusterNo,dataRegionOffset,dir_name,nextCluster,x))
        {
            clusterNo = nextCluster;
            clusterOffset = bootSector.BytesPerSector * bootSector.SectorsPerCluster * (clusterNo - 2);
        }
        else
        {
            return false;
        }
    }
    bool find_ok = generalFinder(img,clusterNo,dataRegionOffset,textToRead,nextCluster,x);
    printFile(nextCluster,img,dataRegionOffset,x);
    return find_ok;
}


// mkdir command(same as linux)
void mkdirCommand(FILE * img,uint32_t dataRegionOffset,std::vector<std::string> path,std::string folder_name)
{

    uint32_t clusterNo = bootSector.extended.RootCluster;
    int path_size = path.size();
    bool flag = true;
    bool seek_ok1 = (fseek(img,dataRegionOffset,SEEK_SET) == 0);
    uint32_t findedClusterNo;
    uint32_t parentCluster = bootSector.extended.RootCluster;
    uint32_t modCluster = bootSector.extended.RootCluster;
    std::string abcd;
    for(int i = 0; i < path.size();i++)
    {
        std::string dir_name = path[i];
        uint32_t size;
        if(generalFinder(img,clusterNo,dataRegionOffset,dir_name,findedClusterNo,size))
        {
            clusterNo = findedClusterNo;
            parentCluster = findedClusterNo;
            if(i == path_size - 2)
            {
                modCluster = findedClusterNo;
                abcd = dir_name;
            }
        }
        else
        {
            flag = false;
            break;
        }
    }

    clusterNo = tillIterateToEnd(img,dataRegionOffset,clusterNo);
    if(flag)
    {
        
        if(path_size != 0) changeDateAndTime(img,modCluster,dataRegionOffset,path[path_size - 1]);
        FatFile83 folderEntity = createFolderEntity(clusterNo,dataRegionOffset,img,true,parentCluster);

        uint8_t idx = numberOfFolders(img,clusterNo,dataRegionOffset);
        char tmp_var[11];
        for(int i = 0;i < 8; i++)
            tmp_var[i] = folderEntity.filename[i];
        
        for(int i = 0;i < 3;i++)
            tmp_var[i + 8] = folderEntity.extension[i];

        const unsigned char * p = (const unsigned char *) tmp_var;
        unsigned char checksum = lfn_checksum(p);
        std::vector<FatFileLFN> lfns = createLFNs(folder_name,checksum);
        
        int size = lfns.size() + 1;
        uint32_t entrySizeInCluster = (bootSector.BytesPerSector * bootSector.SectorsPerCluster / 32);
        uint32_t contiguousAllocStart = findEmptyEntryInGivenCluster(img,clusterNo,dataRegionOffset,size);
        if(contiguousAllocStart < entrySizeInCluster)
        {   
            int remainingPart = (entrySizeInCluster - contiguousAllocStart) - size;
            if(remainingPart >= 0)
            {
                int i = contiguousAllocStart;
                uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
                uint32_t clusterOffset = clusterSizeInBytes * (clusterNo - 2);
                for(int j = size - 2; j >= 0;j--,i++)
                {
                    bool seek_ok2 = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
                    bool write_ok2 = (fwrite(&lfns[j],sizeof(FatFileLFN),1,img));
                }
                bool seek_ok3 = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
                bool write_ok3 = (fwrite(&folderEntity,sizeof(FatFile83),1,img) > 0);
            }
            else
            {
                int i = contiguousAllocStart;
                int j = size -2;
                uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
                uint32_t clusterOffset = clusterSizeInBytes * (clusterNo - 2);
                for(;j > size - 1 + remainingPart;j--,i++)
                {
                    bool seek_ok = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
                    bool write_ok = (fwrite(&lfns[j],sizeof(FatFileLFN),1,img));
                }
                uint32_t emptyClusterNumber = findEmptyClusterFromFAT(img);
                putClusterNumberToFat(emptyClusterNumber,clusterNo,img);
                putClusterNumberToFat(0x0FFFFFF8,emptyClusterNumber,img);
                clusterNo = emptyClusterNumber;
                
                clusterOffset = clusterSizeInBytes * (clusterNo - 2);
                i = findEmptyEntryInGivenCluster(img,clusterNo,dataRegionOffset,size);
                for(;j >= 0;j--,i++)
                {
                    bool seek_ok = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
                    bool write_ok = (fwrite(&lfns[j],sizeof(FatFileLFN),1,img));
                }
                bool seek_ok = (fseek(img,clusterNo + i*32,SEEK_SET) == 0);
                bool write_ok = (fwrite(&folderEntity,sizeof(FatFile83),1,img));
            }
        }
        else
        {
            uint32_t emptyClusterNumber = findEmptyClusterFromFAT(img);
            putClusterNumberToFat(emptyClusterNumber,clusterNo,img);
            putClusterNumberToFat(0x0FFFFFF8,emptyClusterNumber,img);
            clusterNo = emptyClusterNumber;
            uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
            uint32_t clusterOffset = clusterSizeInBytes * (clusterNo - 2);
            int i =findEmptyEntryInGivenCluster(img,clusterNo,dataRegionOffset,size);
            for(int j = size - 2; j >= 0;j--,i++)
            {
                bool seek_ok = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
                bool write_ok = (fwrite(&lfns[j],sizeof(FatFileLFN),1,img));
            }
            bool seek_ok = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
            bool write_ok = (fwrite(&folderEntity,sizeof(FatFile83),1,img));
        }
    }
}


// touch command(same as linux)
void touchCommand(FILE * img,uint32_t dataRegionOffset,std::vector<std::string> path,std::string folder_name)
{
    uint32_t clusterNo = bootSector.extended.RootCluster;
    int path_size = path.size();
    bool flag = true;
    bool seek_ok1 = (fseek(img,dataRegionOffset,SEEK_SET) == 0);
    uint32_t findedClusterNo;
    uint32_t parentCluster = bootSector.extended.RootCluster;
    uint32_t modCluster = bootSector.extended.RootCluster;
    std::string abcd;
    for(int i = 0; i < path.size();i++)
    {
        std::string dir_name = path[i];
        uint32_t size;
        if(generalFinder(img,clusterNo,dataRegionOffset,dir_name,findedClusterNo,size))
        {
            clusterNo = findedClusterNo;
            parentCluster = findedClusterNo;
             if(i == path_size - 2)
            {
                modCluster = findedClusterNo;
                abcd = dir_name;
            }
        }
        else
        {
            flag = false;
            break;
        }
    }
    clusterNo = tillIterateToEnd(img,dataRegionOffset,clusterNo);
    if(flag)
    {
        if(path_size != 0) changeDateAndTime(img,modCluster,dataRegionOffset,path[path_size - 1]);
        FatFile83 folderEntity = createFolderEntity(clusterNo,dataRegionOffset,img,true,parentCluster);

        uint8_t idx = numberOfFolders(img,clusterNo,dataRegionOffset);
        char tmp_var[11];
        for(int i = 0;i < 8; i++)
            tmp_var[i] = folderEntity.filename[i];
        
        for(int i = 0;i < 3;i++)
            tmp_var[i + 8] = folderEntity.extension[i];

        const unsigned char * p = (const unsigned char *) tmp_var;
        unsigned char checksum = lfn_checksum(p);
        std::vector<FatFileLFN> lfns = createLFNs(folder_name,checksum);
        
        int size = lfns.size() + 1;
        uint32_t entrySizeInCluster = (bootSector.BytesPerSector * bootSector.SectorsPerCluster / 32);
        uint32_t contiguousAllocStart = findEmptyEntryInGivenCluster(img,clusterNo,dataRegionOffset,size);
        if(contiguousAllocStart < entrySizeInCluster)
        {   
            int remainingPart = (entrySizeInCluster - contiguousAllocStart) - size;
            if(remainingPart >= 0)
            {
                int i = contiguousAllocStart;
                uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
                uint32_t clusterOffset = clusterSizeInBytes * (clusterNo - 2);
                for(int j = size - 2; j >= 0;j--,i++)
                {
                    bool seek_ok2 = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
                    bool write_ok2 = (fwrite(&lfns[j],sizeof(FatFileLFN),1,img));
                }
                bool seek_ok3 = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
                bool write_ok3 = (fwrite(&folderEntity,sizeof(FatFile83),1,img) > 0);
            }
            else
            {
                int i = contiguousAllocStart;
                int j = size -2;
                uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
                uint32_t clusterOffset = clusterSizeInBytes * (clusterNo - 2);
                for(;j > size - 1 + remainingPart;j--,i++)
                {
                    bool seek_ok = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
                    bool write_ok = (fwrite(&lfns[j],sizeof(FatFileLFN),1,img));
                }
                uint32_t emptyClusterNumber = findEmptyClusterFromFAT(img);
                putClusterNumberToFat(emptyClusterNumber,clusterNo,img);
                putClusterNumberToFat(0x0FFFFFF8,emptyClusterNumber,img);
                clusterNo = emptyClusterNumber;
                
                clusterOffset = clusterSizeInBytes * (clusterNo - 2);
                i = findEmptyEntryInGivenCluster(img,clusterNo,dataRegionOffset,size);
                for(;j >= 0;j--,i++)
                {
                    bool seek_ok = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
                    bool write_ok = (fwrite(&lfns[j],sizeof(FatFileLFN),1,img));
                }
                bool seek_ok = (fseek(img,clusterNo + i*32,SEEK_SET) == 0);
                bool write_ok = (fwrite(&folderEntity,sizeof(FatFile83),1,img));
            }
        }
        else
        {
            uint32_t emptyClusterNumber = findEmptyClusterFromFAT(img);
            putClusterNumberToFat(emptyClusterNumber,clusterNo,img);
            putClusterNumberToFat(0x0FFFFFF8,emptyClusterNumber,img);
            clusterNo = emptyClusterNumber;
            uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
            uint32_t clusterOffset = clusterSizeInBytes * (clusterNo - 2);
            int i =findEmptyEntryInGivenCluster(img,clusterNo,dataRegionOffset,size);
            for(int j = size - 2; j >= 0;j--,i++)
            {
                bool seek_ok = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
                bool write_ok = (fwrite(&lfns[j],sizeof(FatFileLFN),1,img));
            }
            bool seek_ok = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
            bool write_ok = (fwrite(&folderEntity,sizeof(FatFile83),1,img));
        }
    }
}


// Used by mv command to create new entry in that directory.
void mkdirForMove(FILE * img, FatFile83 fileEntry, 
                  std::vector<FatFileEntry *> LFNs, uint32_t dataRegionOffset
                  ,uint32_t destinationCluster)
{
    uint8_t folderNumber = numberOfFolders(img,destinationCluster,dataRegionOffset);
    destinationCluster = tillIterateToEnd(img,dataRegionOffset,destinationCluster);

    
    fileEntry.filename[0] = '~';
    int idx = 1;
    std::vector<uint8_t> tmpx;

    
    while(folderNumber)
    {
        uint8_t digit = folderNumber % 10;
        tmpx.push_back(48 + digit);
        folderNumber /= 10;
    }

    for(int i = tmpx.size() - 1;i>= 0;i--)
        fileEntry.filename[idx++] = tmpx[i];

    while(idx < 8)
        fileEntry.filename[idx++] = ' ';

    

    int size = LFNs.size() + 1;
    //Search for an empty entry in clusterNo. If it is, then 
    uint32_t entrySizeInCluster = (bootSector.BytesPerSector * bootSector.SectorsPerCluster / 32);
    uint32_t contiguousAllocStart = findEmptyEntryInGivenCluster(img,destinationCluster,dataRegionOffset,size);
    if(contiguousAllocStart < entrySizeInCluster)
    {   
        //Fat allocladıktan sonra cluster chain için indexini güncelle.
        int remainingPart = (entrySizeInCluster - contiguousAllocStart) - size;
        if(remainingPart >= 0)
        {
            // Alan mkdir için yetti. contiguousAlloc variable dan sonraki kısımları doldur.
            int i = contiguousAllocStart;
            uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
            uint32_t clusterOffset = clusterSizeInBytes * (destinationCluster - 2);
            for(int j = size - 2; j >= 0;j--,i++)
            {
                bool seek_ok2 = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
                bool write_ok2 = (fwrite(&(LFNs[j]->lfn),sizeof(FatFileLFN),1,img));
            }
            // fseek olayları. unutma
            bool seek_ok3 = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
            bool write_ok3 = (fwrite(&fileEntry,sizeof(FatFile83),1,img) > 0);
        }
        else
        {
            // Alan yetmedi koyabildiğini koy koyamadığını diğer cluster a at.
            int i = contiguousAllocStart;
            int j = size -2;
            uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
            uint32_t clusterOffset = clusterSizeInBytes * (destinationCluster - 2);
            for(;j > size - 1 + remainingPart;j--,i++)
            {
                bool seek_ok = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
                bool write_ok = (fwrite(&(LFNs[j]->lfn),sizeof(FatFileLFN),1,img));
            }
            uint32_t emptyClusterNumber = findEmptyClusterFromFAT(img);
            putClusterNumberToFat(emptyClusterNumber,destinationCluster,img);
            putClusterNumberToFat(0x0FFFFFF8,emptyClusterNumber,img);
            destinationCluster = emptyClusterNumber;
            
            clusterOffset = clusterSizeInBytes * (destinationCluster - 2);
            i = findEmptyEntryInGivenCluster(img,destinationCluster,dataRegionOffset,size);
            for(;j >= 0;j--,i++)
            {
                bool seek_ok = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
                bool write_ok = (fwrite(&(LFNs[j]->lfn),sizeof(FatFileLFN),1,img));
            }
            bool seek_ok = (fseek(img,destinationCluster + i*32,SEEK_SET) == 0);
            bool write_ok = (fwrite(&fileEntry,sizeof(FatFile83),1,img));
        }
    }
    else
    {
        // Full dolu cluster bundan kaynaklı yeni bir yer allocluyoz.
        uint32_t emptyClusterNumber = findEmptyClusterFromFAT(img);
        putClusterNumberToFat(emptyClusterNumber,destinationCluster,img);
        putClusterNumberToFat(0x0FFFFFF8,emptyClusterNumber,img);
        destinationCluster = emptyClusterNumber;
        uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
        uint32_t clusterOffset = clusterSizeInBytes * (destinationCluster - 2);
        int i =findEmptyEntryInGivenCluster(img,destinationCluster,dataRegionOffset,size);
        for(int j = size - 2; j >= 0;j--,i++)
        {
            bool seek_ok = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
            bool write_ok = (fwrite(&(LFNs[j]->lfn),sizeof(FatFileLFN),1,img));
        }
        bool seek_ok = (fseek(img,clusterOffset + dataRegionOffset + i*32,SEEK_SET) == 0);
        bool write_ok = (fwrite(&fileEntry,sizeof(FatFile83),1,img));
    }

    for(int i = 0; i < LFNs.size();i++)
        delete LFNs[i];
}


// mv command (same as Linux)
void moveCommand(FILE * img, uint32_t dataRegionOffset, 
                std::string toMovedName,std::vector<std::string> sourcePath,
                std::vector<std::string> destinationPath)
{

    uint32_t clusterNo = bootSector.extended.RootCluster;
    
    bool flag = true;
    bool seek_ok = (fseek(img,dataRegionOffset,SEEK_SET) == 0);
    uint32_t modCluster = bootSector.extended.RootCluster;
    std::string abcd;
    for(int i = 0; i < sourcePath.size();i++)
    {
        std::string dir_name = sourcePath[i];
        uint32_t size;
        uint32_t findedClusterNo;
        if(generalFinder(img,clusterNo,dataRegionOffset,dir_name,findedClusterNo,size))
        {
            clusterNo = findedClusterNo;
            if(i == sourcePath.size() - 2)
            {
                modCluster = findedClusterNo;
                abcd = dir_name;
            }

        }
        else
        {
            flag = false;
            break;
        }
    }
    if(flag)
    {
        bool secondFlag = true;
        uint32_t destinationClusterNo = bootSector.extended.RootCluster;
        int path_size = destinationPath.size();
        bool seek_ok = (fseek(img,dataRegionOffset,SEEK_SET) == 0);

        uint32_t modCluster2 = bootSector.extended.RootCluster;
        std::string abcd2;

        for(int i = 0; i < path_size;i++)
        {
            std::string destinationDirName = destinationPath[i];
            uint32_t size;
            uint32_t findedClusterNo;

            if(generalFinder(img,destinationClusterNo,dataRegionOffset,destinationDirName,findedClusterNo,size))
            {
                destinationClusterNo = findedClusterNo;

                if(i == sourcePath.size() - 1)
                {
                    modCluster2 = findedClusterNo;
                    abcd2 = destinationDirName;
                }

            }
            else
            {
                secondFlag = false;
                break;
            }

        }
        uint32_t abc,def;
        if(generalFinder(img,destinationClusterNo,dataRegionOffset,toMovedName,abc,def)) return;
        if(secondFlag)
        {
            
            if(sourcePath.size() != 0) 
                changeDateAndTime(img,modCluster,dataRegionOffset,sourcePath[sourcePath.size() - 1]);
            if(destinationPath.size() != 0)
                changeDateAndTime(img,modCluster2,dataRegionOffset,destinationPath[destinationPath.size() - 1]);

            std::vector <FatFileEntry *> LFNs = getLFNs(img,toMovedName,dataRegionOffset,clusterNo);
            mkdirForMove(img,fileEntryToMoved,LFNs,dataRegionOffset,destinationClusterNo);
        }

    }
}

#endif
