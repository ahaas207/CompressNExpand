/*  Name: Aimee Haas
    2-2022
    Purpose:  Compress 8 bit ASCII into 7. 
    Decompress back to 8 bit representation. 
    */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int compress(int fp, char *filename);
int decompress(int fp, char *filename);
int writeBuffer2File(int compressed, char *filename, int fp);

int fileSize;
//  Read buffer
unsigned char* rBuf;
//  Write buffer
unsigned char* wBuf;

/*  Uses unsigned long long int for seamless processing.
    Compression: 64 - (1 * 8) = 56. Leaves one empty byte to cull.
    Decompression: 56 + (1 * 8) = 64. All bytes in use. */  
unsigned long long sixtyFourBitBuf = 0;
int numShift = 0, rBufIndex = 0, wBufIndex = 0;
unsigned int maskEightOnes = 0xFF, maskSevenOnes = 0x7f;

int main(int argc, char **argv)
{   
    // Ensures there is a file argument
    if(argc < 2)
    {
        fprintf(stderr,"Expected a file. \n");
        exit(1);
    }


    int fp = open(argv[1], O_RDONLY);

    // Ensures file opens. 
    if (fp < 0)
    {
        fprintf(stderr,"Could not open file. \n");
        exit(1);
    }

    // Stores file size, returns pointer to start of file.
    fileSize = lseek(fp, 0, SEEK_END);
    lseek(fp, 0, SEEK_SET);

    // Allocate read buffer, cast array of char pointers. 
    rBuf = (unsigned char*)malloc(fileSize);
    int fileR = read(fp, rBuf, fileSize);
 
    if (!fileR)
    {
        fprintf(stderr,"File error. \n");
        close(fp);
        free(rBuf);
        exit(1);
    }
    
    /*  Compress or decompress based on last file extension.
        Compressed files will have two extensions
        e.g. short.txt.z827 */ 
    char * fileEnding = strrchr(argv[1], '.');
    if (strcmp(fileEnding, ".z827")==0)
        decompress(fp, argv[1]);
    else
        compress(fp, argv[1]);

}

int compress(int fp, char *fileName)
{
    // Allocate write buffer, cast array of char pointers.  
    wBuf = (unsigned char*)malloc(fileSize);

    while(rBufIndex < fileSize)
    {
        // Check for signed chars (8 bits).
        if((rBuf[rBufIndex] & 0x80) == 0x80)
        {
            fprintf(stderr,"Not compressible.\n");
            close(fp);
            free(rBuf);
            exit(1);
        }
        /*  Casts the next rBuf byte to long long
            to accomodate the or bitwise operation. */
        sixtyFourBitBuf |= 
            ((unsigned long long)rBuf[rBufIndex++]) << numShift;
        numShift +=7;

        if (numShift == 56)
        {
            //Move 7 completed bytes to write buffer. 
            while (numShift)
            {
                wBuf[wBufIndex++] |= sixtyFourBitBuf & maskEightOnes;
                sixtyFourBitBuf >>= 8;
                numShift -= 8;
            }
            // Reset for next batch of 8 bytes. 
            sixtyFourBitBuf = 0;
        }    
    }
    //Case: less than 7 bytes are left in sixtyFourBitBuf 
    while (numShift > 0)
    {
        wBuf[wBufIndex++] |= sixtyFourBitBuf & maskEightOnes;
        sixtyFourBitBuf >>= 8;
        numShift -= 8;
    }
    writeBuffer2File(1, fileName, fp);
}

int decompress(int fp, char *fileName)
{
    int origFileSize = (int)rBuf[0];
    rBufIndex = 4;
    // Allocate write buffer, cast array of char pointers.  
    wBuf = (unsigned char*)malloc(fileSize + fileSize/7);
    while(rBufIndex < fileSize)
    {
        /*  Casts the next rBuf byte to long long
            to accomodate the or bitwise operation. */
        sixtyFourBitBuf |= 
            ((unsigned long long)rBuf[rBufIndex++]) << numShift;
        numShift +=8;

        if (numShift == 56)
        {
            //Move 8 completed bytes to write buffer. 
            while (numShift > 0)
            {
                wBuf[wBufIndex++] |= sixtyFourBitBuf & maskSevenOnes;
                sixtyFourBitBuf >>= 7;
                numShift -= 7;
            }
            // Reset for next batch of 8 bytes. 
            sixtyFourBitBuf = 0;
        }    
    }
    numShift -=7;
    //Case: less than 7 bytes are left in sixtyFourBitBuf 
    while (numShift > 0)
    {
        wBuf[wBufIndex++] |= sixtyFourBitBuf & maskSevenOnes;
        sixtyFourBitBuf >>= 7;
        numShift -= 7;
    }

    writeBuffer2File(0, fileName, fp);

}

/*  Either append or remove the extention for the file 
    depending of if it is compressed or decompressed. 
    If compressed, the ending .z827 will be added to the end.
    If decompressed, the ending .z827 will be removed.  
    This function then removes the original file
    and frees the dynamically allocated memory when complete. */

int writeBuffer2File(int compressed, char *fileName, int fp)
{
    int outFp;
    char origFileName[40];
    strcpy(origFileName, fileName);
    if(compressed)
    {
        char *fileNameWithExt = malloc(strlen(fileName) + 6);
        fileNameWithExt = strcat(fileName, ".z827");
        outFp = creat(fileNameWithExt, 0644);
        if (!write(outFp, &fileSize, 4))
        {
            fprintf(stderr,"File write error 1. \n");
            close(fp);
            free(rBuf);
            free(wBuf);
            exit(1);
        }

    }
    else
    {
        char *newFileName = malloc(strlen(fileName) + 1);
        strncpy(newFileName, fileName, (strlen(fileName) - 5));
        newFileName[strlen(fileName) - 5] = '\0';
        outFp = creat(newFileName, 0644);
    }
    if (!write(outFp, wBuf, wBufIndex))
    {
        fprintf(stderr,"File write error 2. \n");
        close(fp);
        free(rBuf);
        free(wBuf);
        exit(1);
    };
    close(outFp);
    unlink(origFileName);
    free(rBuf);
    free(wBuf);
}
