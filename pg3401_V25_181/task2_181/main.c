#include <stdio.h>
#include <string.h>
#include "main.h"
#include "task2_count.h"
#include "task2_hash.h"
#include "task2_sum.h"

int main(void) {
    const char *inputName;
	inputName = "pgexam25_test.txt";
    const char *outputName ;
	outputName = "pgexam25_output.bin";
	//Open input file
    FILE *file = fopen(inputName, "r");
    if (!file) {
        fprintf(stderr, "Error: cannot open \"%s\" for reading.\n", inputName);
        return 1;
    }
	
    struct TASK2_FILE_METADATA fileInfo;  
	//Copy filename to the struct
    strncpy(fileInfo.szFileName, inputName, sizeof(fileInfo.szFileName) - 1);
    fileInfo.szFileName[sizeof(fileInfo.szFileName) - 1] = '\0';

	//Calls the funtions provided by EWA
    Task2_CountEachCharacter(file, fileInfo.aAlphaCount);
    Task2_SimpleDjb2Hash(file, fileInfo.byHash);
    Task2_SizeAndSumOfCharacters(file, &fileInfo.iFileSize, &fileInfo.iSumOfChars);

	//Prints info from the struct
    printf("File Name:%s\n", fileInfo.szFileName);
    printf("File Size:%d bytes\n", fileInfo.iFileSize);
    printf("Sum of Chars:%d\n", fileInfo.iSumOfChars);
    printf("DJB2 Hash: %02x%02x%02x%02x\n",
           (unsigned char)fileInfo.byHash[0],
           (unsigned char)fileInfo.byHash[1],
           (unsigned char)fileInfo.byHash[2],
           (unsigned char)fileInfo.byHash[3]);
	//Create binary file or overwrite (if the file exists)
    FILE *bin = fopen(outputName, "wb");
    if (!bin) {
        fprintf(stderr, "Error: cannot open \"%s\" for writing.\n", outputName);
        fclose(file);
        return 1;
    }
	//Write to binary file
    fwrite(&fileInfo, sizeof(struct TASK2_FILE_METADATA), 1, bin);
	//Closes both input and output files
    fclose(bin);
    fclose(file);
    return 0;
}
