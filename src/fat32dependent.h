#ifndef FAT32DEPENDENT_H
#define FAT32DEPENDENT_H
// FAT32 DEPENDENT FUNCTIONS. They are used by commands.cpp functions.
#include "includes.h"
#include "attribute.h"


// Calculates next chain from FAT.
uint32_t calculateNextChainIdx(FILE * img,uint32_t currClusterNumber)
{
    uint32_t mask = 0x0FFFFFFF;
    uint32_t reservedSectorInBytes = bootSector.ReservedSectorCount * bootSector.BytesPerSector;
    uint32_t fatEntryAddr = reservedSectorInBytes + (currClusterNumber) * 4;
    uint32_t fatEntry;
    bool seek_ok = (fseek(img,fatEntryAddr,SEEK_SET) == 0);
    bool read_ok = (fread(&fatEntry,sizeof(fatEntry),1,img) != 0);
    return fatEntry & mask;
}

// Cheksum calculator for fat32 entry.
unsigned char lfn_checksum(const unsigned char * name)
{
    int i;
    unsigned char sum = 0;
    for(i = 11;i;i--)
        sum = (((sum & 1)) << 7) + (sum >> 1) + *name++;

    return sum;
}


// Lfn creation for new folder and files.
std::vector<FatFileLFN> createLFNs(std::string name,unsigned char checksum)
{
    std::vector<FatFileLFN> list;
    FatFileLFN * tmp = new FatFileLFN;
    tmp->attributes = 0x0F;
    tmp->reserved = 0x00;
    tmp->firstCluster = 0x00;
    int idx1 = 0,
        idx2 = 0,
        idx3 = 0;

    tmp->checksum = checksum;

    for(int i = 0; i < name.size();i++)
    {
        if(idx1 < 5)
        {
            tmp->name1[idx1++] = name[i];
        }
        else if(idx2 < 6)
        {
            tmp->name2[idx2++] = name[i];
        } 
        else
        {
            if(idx3 < 2)
            {
                tmp->name3[idx3++] = name[i];
            } 
            else{
                list.push_back(*tmp);
                idx1 = 0;
                idx2 = 0;
                idx3 = 0;
                delete tmp;
                tmp = new FatFileLFN;
                tmp->checksum = checksum;
                tmp->attributes = 0x0F;
                tmp->reserved = 0x00;
                tmp->firstCluster = 0x00;
                tmp->name1[idx1++] = name[i];
            }
        }
    }
    if(idx1 < 5)
    {
        tmp->name1[idx1++] = '\0';
        list.push_back(*tmp);
    }
    else if(idx2 < 6)
    {
        tmp->name2[idx2++] = '\0';
        list.push_back(*tmp);
    }
    else if(idx3 < 2)
    {
        tmp->name3[idx3++] = '\0';
        list.push_back(*tmp);
    }
    else
    {
        list.push_back(*tmp);
        FatFileLFN * last = new FatFileLFN;
        last->checksum = checksum;
        last->attributes = 0x0F;
        last->reserved = 0x00;
        last->firstCluster = 0x00;
        last->name1[0] = '\0';
        list.push_back(*last);
        delete last;
    }
    delete tmp;

    uint8_t seq_no_first = 0x40 + list.size();
    list[list.size() - 1].sequence_number = seq_no_first;

    uint8_t rest = list.size() - 1;
    for(int i = list.size() - 2; i >= 0;i--)
        list[i].sequence_number = (rest--);
    return list;
}


// This function calculates number of folders and files in a directory. This number
// is used in fat32 short name field.
uint8_t numberOfFolders(FILE * img, uint32_t clusterNo, uint32_t dataRegionOffset)
{
    //Ufak bir hata var.
    int numberOfChain = 0;
    uint8_t numberOfEntries = 0;
    bool flag = false;
    if(clusterNo == bootSector.extended.RootCluster) flag = true;

    uint32_t currClusterNo = clusterNo;
    uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
    uint32_t clusterOffset = clusterSizeInBytes * (currClusterNo - 2);

    bool fseek_ok = (fseek(img,dataRegionOffset + clusterOffset,SEEK_SET) == 0);
    uint32_t entryCount = 0;

    while(true)
    {
        FatFileEntry entry;
        bool read_ok = fread(&entry,sizeof(entry),1,img);
        entryCount++;

        if(entry.lfn.attributes == 0x00)
        {
            uint32_t nextCluster = calculateNextChainIdx(img,currClusterNo);

            if(nextCluster >= 0x0FFFFFF8)
            {
                if(flag)   return numberOfEntries + 1;
                else return numberOfEntries - 1;
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
        else if(entry.msdos.attributes == 0x10 || entry.msdos.attributes == 0x20)
        {
            numberOfEntries++;
        }
        if(entryCount == (bootSector.BytesPerSector * bootSector.SectorsPerCluster / 32))
        {
            entryCount = 0;
            uint32_t nextIdx = calculateNextChainIdx(img,currClusterNo);
            if(nextIdx >= 0x0FFFFFF8){
                if(flag)   return numberOfEntries + 1;
                else return numberOfEntries - 1;
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
    return numberOfEntries;
}


// returns last cluster number of the folder.
uint32_t tillIterateToEnd(FILE * img, uint32_t dataRegionOffset,uint32_t clusterNo)
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
                return currClusterNo;
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
        }
        if(entryCount == (bootSector.BytesPerSector * bootSector.SectorsPerCluster / 32))
        {
            entryCount = 0;
            uint32_t nextIdx = calculateNextChainIdx(img,currClusterNo);
            if(nextIdx >= 0x0FFFFFF8){
                return currClusterNo;
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
    return 0xFFFFFFFF;
}


// Used for finding new free cluster.
uint32_t findEmptyClusterFromFAT(FILE * img)
{
    
    uint32_t reservedSectorInBytes = bootSector.ReservedSectorCount * bootSector.BytesPerSector;
    uint32_t FATaddr = reservedSectorInBytes;
    uint32_t entryCount = 0;
    uint32_t entry;

    bool seek_ok = (fseek(img,FATaddr,SEEK_SET) == 0);

    while (true)
    {
        bool read_ok = (fread(&entry,sizeof(entry),1,img) != 0);
        entryCount++;

        if(entry == 0x00000000)
        {
            return entryCount - 1;
        }
        seek_ok = (fseek(img,FATaddr + entryCount * 4,SEEK_SET) == 0);
    }
    return 0xFFFFFFFF;
}

// Organize cluster chains in fat32.
// For example I put 0x0FFFFFF8 to specify end of chain for a file.
void putClusterNumberToFat(uint32_t toBePut, uint32_t location,FILE * img)
{
    
    uint32_t reservedSectorInBytes = bootSector.ReservedSectorCount * bootSector.BytesPerSector;
    uint32_t FATaddr = reservedSectorInBytes + location * 4;

    bool seek_ok = (fseek(img,FATaddr, SEEK_SET) == 0);
    bool write_ok = (fwrite(&toBePut,sizeof(toBePut),1,img) > 0);

    return;
}


// If current directory full we need to allocate new cluster 
// to create our folder/file so if this is a newly creating folder
// we need to put . and .. folders to new folder.
void putDefaultDirs(FILE * img, uint32_t dataRegionOffset,uint32_t clusterNo,uint32_t parentCluster)
{

    FatFile83 x;
    x.attributes = 0x10;
    x.filename[0] = '.';
    x.filename[1] = ' ';
    x.filename[2] = ' ';
    x.filename[3] = ' ';
    x.filename[4] = ' ';
    x.filename[5] = ' ';
    x.filename[6] = ' ';
    x.filename[7] = ' ';

    FatFile83 y;
    y.attributes = 0x10;
    y.filename[0] = '.';
    y.filename[1] = '.';
    y.filename[2] = ' ';
    y.filename[3] = ' ';
    y.filename[4] = ' ';
    y.filename[5] = ' ';
    y.filename[6] = ' ';
    y.filename[7] = ' ';
    
    uint16_t eaIdx = (parentCluster >> 16);

    y.eaIndex = eaIdx;
    y.firstCluster = parentCluster & 0x0000FFFF;

    auto creationTime = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(creationTime);
    struct tm *aTime = localtime(&end_time);

    uint8_t day = aTime ->tm_mday;
    uint8_t month = aTime->tm_mon + 1;
    uint8_t year = aTime ->tm_year - 80;
    uint16_t modifiedDate = (year << 9);
    modifiedDate = modifiedDate | (month << 5);
    modifiedDate = modifiedDate | (day);

    uint8_t hour = aTime ->tm_hour;
    uint8_t min = aTime -> tm_min;
    uint8_t sec = (aTime ->tm_sec) >> 1;

    uint16_t modifiedTime = (hour << 11);
    modifiedTime = modifiedTime | (min << 6);
    modifiedTime = modifiedTime | sec;

    x.creationDate = y.creationDate = modifiedDate;
    x.modifiedDate = y.modifiedDate = modifiedDate;
    
    x.creationTime = y.creationDate =modifiedTime;
    x.modifiedTime = y.modifiedDate = modifiedTime;


    int i = 0;
    uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
    uint32_t clusterOffset = clusterSizeInBytes * (clusterNo - 2);

    bool seek_ok = (fseek(img,dataRegionOffset + clusterOffset,SEEK_SET) == 0);
    bool write_ok =  (fwrite(&x,sizeof(FatFile83),1,img));

    seek_ok = (fseek(img,dataRegionOffset + clusterOffset + 32,SEEK_SET) == 0);
    write_ok =  (fwrite(&y,sizeof(FatFile83),1,img));

}

// Creates FatFie83 struct.
FatFile83 createFolderEntity(uint32_t clusterNo,uint32_t datRegionOffset,FILE * img,bool flag,uint32_t parentNo)
{
    FatFile83 folderEntity;

    
    if(flag) folderEntity.attributes = 0x10;
    else folderEntity.attributes = 0x20;

    folderEntity.reserved = 0x00;
    folderEntity.fileSize = 0x00;
    folderEntity.creationTimeMs = 0x00;

    if(flag)
    {
        uint32_t clusterIdx = findEmptyClusterFromFAT(img);
        putClusterNumberToFat(0x0FFFFFF8 ,clusterIdx,img);
        uint16_t eaIdx = (clusterIdx >> 16);
        folderEntity.eaIndex = eaIdx;
        folderEntity.firstCluster = clusterIdx & 0x0000FFFF;


        uint16_t sectorInBytes = bootSector.BytesPerSector;
        uint8_t numbOfFats = bootSector.NumFATs;
        uint32_t fatInSectors = bootSector.extended.FATSize;
        uint16_t rezSectorCount = bootSector.ReservedSectorCount;

        uint32_t dataRegionOffset = numbOfFats * fatInSectors * sectorInBytes 
                                + rezSectorCount * sectorInBytes;
    }
    else
    {
        folderEntity.eaIndex = 0;
        folderEntity.firstCluster = 0;
    }

    folderEntity.extension[0] = ' ';
    folderEntity.extension[1] = ' ';
    folderEntity.extension[2] = ' ';


    folderEntity.lastAccessTime = 0x0000;
    auto creationTime = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(creationTime);
    struct tm *aTime = localtime(&end_time);

    uint8_t day = aTime ->tm_mday;
    uint8_t month = aTime->tm_mon + 1;
    uint8_t year = aTime ->tm_year - 80;
    uint16_t modifiedDate = (year << 9);
    modifiedDate = modifiedDate | (month << 5);
    modifiedDate = modifiedDate | (day);

    uint8_t hour = aTime ->tm_hour;
    uint8_t min = aTime -> tm_min;
    uint8_t sec = (aTime ->tm_sec) >> 1;

    uint16_t modifiedTime = (hour << 11);
    modifiedTime = modifiedTime | (min << 5);
    modifiedTime = modifiedTime | sec;

    folderEntity.creationDate = modifiedDate;
    folderEntity.modifiedDate = modifiedDate;
    
    folderEntity.creationTime = modifiedTime;
    folderEntity.modifiedTime = modifiedTime;

    uint8_t folderNumber = (char) numberOfFolders(img,parentNo,datRegionOffset);
    
    folderEntity.filename[0] = '~';
    int idx = 1;
    std::vector<uint8_t> tmpx;

    while(folderNumber)
    {
        uint8_t digit = folderNumber % 10;
        tmpx.push_back(48 + digit);
        folderNumber /= 10;
    }

    for(int i = tmpx.size() - 1;i>= 0;i--)
        folderEntity.filename[idx++] = tmpx[i];

    while(idx < 8)
        folderEntity.filename[idx++] = ' ';

    return folderEntity;
}


// Seeks empty entry in a cluster.
uint32_t findEmptyEntryInGivenCluster(FILE * img,uint32_t clusterNo,uint32_t dataRegionOffset,int numberOfContiguousArea)
{
    uint32_t currClusterNo = clusterNo;
    uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
    uint32_t clusterOffset = clusterSizeInBytes * (currClusterNo - 2);
    bool fseek_ok = (fseek(img,dataRegionOffset + clusterOffset,SEEK_SET) == 0);
    int contiguousAlloc = 0;
    uint32_t entryCount = 0;
    uint32_t startOffset = (bootSector.BytesPerSector * bootSector.SectorsPerCluster / 32) + 1;
    std::vector<FatFileLFN> LFNs;
    while (true)
    {
        FatFileEntry entry;
        bool read_ok = fread(&entry,sizeof(entry),1,img);
        entryCount++;
        
        // 0xE5 kısmı yanlış olabilir deleted file ları kontrol edior diye düşündüm.
        if(entry.lfn.attributes == 0x00 || entry.msdos.filename[0] == 0xE5 || entry.msdos.filename[0] == 0x00)
        { 
            if(contiguousAlloc == 0)
            {
                contiguousAlloc++;
                startOffset = entryCount - 1;
            }
            else
            {
                contiguousAlloc++;
            }
            if(contiguousAlloc == numberOfContiguousArea)
            {
                return startOffset;
            }
        }
        else
        {
            contiguousAlloc = 0;
            startOffset = (bootSector.BytesPerSector * bootSector.SectorsPerCluster / 32) + 1;
        }
        if(entryCount == (bootSector.BytesPerSector * bootSector.SectorsPerCluster / 32))  break;
        else
            fseek_ok = (fseek(img,dataRegionOffset + entryCount * 32 + clusterOffset,SEEK_SET) == 0);
    }
    return startOffset;
}

// If mv command is used. This function called in order to
// change destination and source path's modification date/time. 
void changeDateAndTime(FILE * img, uint32_t clusterNumb, uint32_t dataRegionOffset,std::string toBeFind)
{

    uint32_t currClusterNo = clusterNumb;
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
                return;
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
                
                auto creationTime = std::chrono::system_clock::now();
                std::time_t end_time = std::chrono::system_clock::to_time_t(creationTime);
                struct tm *aTime = localtime(&end_time);

                uint8_t day = aTime ->tm_mday;
                uint8_t month = aTime->tm_mon + 1;
                uint8_t year = aTime ->tm_year - 80;
                uint16_t modifiedDate = (year << 9);
                modifiedDate = modifiedDate | (month << 5);
                modifiedDate = modifiedDate | (day);

                uint8_t hour = aTime ->tm_hour;
                uint8_t min = aTime -> tm_min;
                uint8_t sec = (aTime ->tm_sec) >> 1;

                uint16_t modifiedTime = (hour << 11);
                modifiedTime = modifiedTime | (min << 5);
                modifiedTime = modifiedTime | sec;


                bool seek_ok = fseek(img, dataRegionOffset + clusterOffset + 32 * (entryCount - 1) + 22,SEEK_SET);
                bool write_ok = (fwrite(&modifiedTime,sizeof(uint16_t),1,img));

                seek_ok = fseek(img, dataRegionOffset + clusterOffset + 32 * (entryCount - 1) + 24,SEEK_SET);
                write_ok = (fwrite(&modifiedDate,sizeof(uint16_t),1,img));
                return;
            }
        }
        else if(entry.msdos.attributes == 0x20)
        {
            std::string tmp;
            for(int iter = LFNs.size() - 1; iter >= 0;iter--)
            {
                FatFileLFN currLFN = LFNs[iter];
                pushToString(tmp, currLFN);
            }
            LFNs.clear();
            
        }
        if(entryCount == (bootSector.BytesPerSector * bootSector.SectorsPerCluster / 32))
        {
            entryCount = 0;
            uint32_t nextIdx = calculateNextChainIdx(img,currClusterNo);
            if(nextIdx >= 0x0FFFFFF8){
                return;
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
    return;
}


// This function used in mv command. Get pointers to fat file entries. 
std::vector<FatFileEntry *> getLFNs(FILE * img,std::string file_name,uint32_t dataRegionOffset, uint32_t clusterNo)
{
    uint32_t currClusterNo = clusterNo;
    uint32_t clusterSizeInBytes = bootSector.BytesPerSector * bootSector.SectorsPerCluster;
    uint32_t clusterOffset = clusterSizeInBytes * (currClusterNo - 2);
    bool fseek_ok = (fseek(img,dataRegionOffset + clusterOffset,SEEK_SET) == 0);
    uint32_t entryCount = 0;
    std::vector<FatFileEntry *> LFNs;
    int start = 0;
    int end = 0;
    while (true)
    {
        FatFileEntry * entry = new FatFileEntry;
        bool read_ok = fread(entry,sizeof(FatFileEntry),1,img);
        entryCount++;
        
        if(entry->lfn.attributes == 0x00)
        {
            delete entry;
            uint32_t nextCluster = calculateNextChainIdx(img,currClusterNo);

            if(nextCluster >= 0x0FFFFFF8)
            {
                return LFNs;

            }
            else 
            {
                currClusterNo = nextCluster;
                entryCount = 0;
                clusterOffset = clusterSizeInBytes * (currClusterNo - 2);
                fseek_ok = (fseek(img,dataRegionOffset + clusterOffset,SEEK_SET) == 0);
                continue;
            }
        }
        else if(entry->lfn.attributes == 0x0F)
        {
            start = entryCount - 1;
            LFNs.push_back(entry);
        }
        else if(entry->msdos.attributes == 0x10)
        {
            std::string tmp;

            for(int iter = LFNs.size() - 1; iter >= 0;iter--)
            {
                FatFileLFN  currLFN = (LFNs[iter]->lfn);
                pushToString(tmp, currLFN);
            }
            if(tmp == file_name)
            {
                end = entryCount - 1;

                uint8_t free = 0xE5;
                for(int x = start; x <= end;x++)
                {
                    fseek_ok = (fseek(img,dataRegionOffset + x * 32 + clusterOffset,SEEK_SET) == 0);
                    bool write_ok = fwrite(&free,sizeof(uint8_t),1,img);
                }

                fileEntryToMoved = entry->msdos;
                delete entry;
                return LFNs;
            }
            else
            {
                delete entry;
                for(int i = 0; i < LFNs.size();i++)
                    delete LFNs[i];
                LFNs.clear();
            } 

        }
        else if(entry->msdos.attributes == 0x20)
        {
            std::string tmp;
            for(int iter = LFNs.size() - 1; iter >= 0;iter--)
            {
                FatFileLFN  currLFN = LFNs[iter]->lfn;
                pushToString(tmp, currLFN);
            }
            if(tmp == file_name)
            {
                end = entryCount - 1;

                uint8_t free = 0xE5;
                for(int x = start; x <= end;x++)
                {
                    fseek_ok = (fseek(img,dataRegionOffset + x * 32 + clusterOffset,SEEK_SET) == 0);
                    bool write_ok = fwrite(&free,sizeof(uint8_t),1,img);
                }

                fileEntryToMoved = entry->msdos;
                delete entry;
                return LFNs;
            }
            else
            {
                delete entry;
                for(int i = 0; i < LFNs.size();i++)
                    delete LFNs[i];
                LFNs.clear();
            }
        }

        if(entryCount == (bootSector.BytesPerSector * bootSector.SectorsPerCluster / 32))
        {
            entryCount = 0;
            uint32_t nextIdx = calculateNextChainIdx(img,currClusterNo);
            if(nextIdx >= 0x0FFFFFF8){
                return LFNs;
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
}



#endif
