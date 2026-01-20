#ifndef MAIN_H
#define MAIN_H

#pragma pack(1) //Reduces the struct size by forcing non padding

struct TASK2_FILE_METADATA {
    char szFileName[32];    
    int iFileSize;
    char byHash[4]; 
    int iSumOfChars;        
    char aAlphaCount[26];
};

#pragma pack()

#endif
