#include <stdio.h>
#include <stdlib.h>
#include "grain.h"
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define SAMPLE_SIZE 25000

typedef struct {
    char iv[17]; // 16 characters for IV (including null terminator)
    char keystream[21]; // 20 characters for keystream (including null terminator)
} DataEntry;

bool isDuplicate(DataEntry entry, DataEntry *data, int size) {
    for (int i = 0; i < size; ++i) {
        if (strcmp(entry.iv, data[i].iv) == 0 && strcmp(entry.keystream, data[i].keystream) == 0) {
            return true; // Found a duplicate
        }
    }
    return false; // No duplicate found
}

void printData(int *IV, int *ks) {
    int i;
    printf("IV:        ");
    for (i = 0; i < 8; ++i) {
        printf("%02x", IV[i]);
    }
    printf("\nKeystream:  ");
    for (i = 0; i < 10; ++i) {
        printf("%02x", ks[i]);
    }
    printf("\n");

}

void transferDataToCSV(int iv[][8], int keystream[][10], int numSamples) {
    FILE *csvFile = fopen("data2.csv", "a");
    if (csvFile == NULL) {
        printf("Error opening file.\n");
        return;
    }

    // Write the column headers
    //fprintf(csvFile, "IV,Keystream\n");

    // Create a set to store unique data entries
    DataEntry *data = malloc(numSamples * sizeof(DataEntry));
    int dataSize = 0;

    // Write the data rows
    for (int i = 0; i < numSamples; ++i) {
        // Convert IV and keystream to strings
        char ivStr[17]; // 16 characters for IV (including null terminator)
        char keystreamStr[21]; // 20 characters for keystream (including null terminator)
        snprintf(ivStr, sizeof(ivStr), "%02x%02x%02x%02x%02x%02x%02x%02x", iv[i][0], iv[i][1], iv[i][2], iv[i][3], iv[i][4], iv[i][5], iv[i][6], iv[i][7]);
        snprintf(keystreamStr, sizeof(keystreamStr), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", keystream[i][0], keystream[i][1], keystream[i][2], keystream[i][3], keystream[i][4], keystream[i][5], keystream[i][6], keystream[i][7], keystream[i][8], keystream[i][9]);

        // Create a new DataEntry struct for the current row
        DataEntry entry;
        strcpy(entry.iv, ivStr);
        strcpy(entry.keystream, keystreamStr);

        // Check if the entry is a duplicate
        if (!isDuplicate(entry, data, dataSize)) {
            // Write IV
            fprintf(csvFile, "%s,", ivStr);

            // Write Keystream
            fprintf(csvFile, "%s", keystreamStr);
            fprintf(csvFile, "\n");

            // Add the entry to the set
            strcpy(data[dataSize].iv, ivStr);
            strcpy(data[dataSize].keystream, keystreamStr);
            dataSize++;
        }
    }

    free(data);
    fclose(csvFile);
    printf("Data transferred to data.csv successfully.\n");
}



int grain_keystream(grain* mygrain) {
	int i,NBit,LBit,outbit;
	/* Calculate feedback and output bits */
	outbit = N(79)^N(78)^N(76)^N(70)^N(49)^N(37)^N(24) ^ X1 ^ X4 ^ X0&X3 ^ X2&X3 ^ X3&X4 ^ X0&X1&X2 ^ X0&X2&X3 ^ X0&X2&X4 ^ X1&X2&X4 ^ X2&X3&X4;
 
	NBit=L(80)^N(18)^N(20)^N(28)^N(35)^N(43)^N(47)^N(52)^N(59)^N(66)^N(71)^N(80)^
			N(17)&N(20) ^ N(43)&N(47) ^ N(65)&N(71) ^ N(20)&N(28)&N(35) ^ N(47)&N(52)&N(59) ^ N(17)&N(35)&N(52)&N(71)^
			N(20)&N(28)&N(43)&N(47) ^ N(17)&N(20)&N(59)&N(65) ^ N(17)&N(20)&N(28)&N(35)&N(43) ^ N(47)&N(52)&N(59)&N(65)&N(71)^
			N(28)&N(35)&N(43)&N(47)&N(52)&N(59);
	LBit=L(18)^L(29)^L(42)^L(57)^L(67)^L(80);
	/* Update registers */
	for (i=1;i<(mygrain->keysize);++i) {
		mygrain->NFSR[i-1]=mygrain->NFSR[i];
		mygrain->LFSR[i-1]=mygrain->LFSR[i];
	}
	mygrain->NFSR[(mygrain->keysize)-1]=NBit;
	mygrain->LFSR[(mygrain->keysize)-1]=LBit;
	return outbit;	
}

int ks[10];
void keystream_bytes(
  grain* mygrain, 
  int* keystream, 
  int msglen)
{
	int i,j;
	for (i = 0; i < msglen; ++i) {
		keystream[i]=0;
		for (j = 0; j < 8; ++j) {
			keystream[i]|=(grain_keystream(mygrain)<<j);
		}
	}
}



void keysetup(
  grain* mygrain, 
  const int* key, 
  int keysize,                /* Key size in bits. */ 
  int ivsize)				  /* IV size in bits. */ 
{
	mygrain->p_key=key;
	mygrain->keysize=keysize;
	mygrain->ivsize=ivsize;
}

void ivsetup(
  grain* mygrain, 
  const int* iv)
{
	int i,j;
	int outbit;
	/* load registers */
	for (i=0;i<(mygrain->ivsize)/8;++i) {
		for (j=0;j<8;++j) {
			mygrain->NFSR[i*8+j]=((mygrain->p_key[i]>>j)&1);  
			mygrain->LFSR[i*8+j]=((iv[i]>>j)&1);
		}
	}
	for (i=(mygrain->ivsize)/8;i<(mygrain->keysize)/8;++i) {
		for (j=0;j<8;++j) {
			mygrain->NFSR[i*8+j]=((mygrain->p_key[i]>>j)&1);
			mygrain->LFSR[i*8+j]=1;
		}
	}
	/* do initial clockings */
	for (i=0;i<INITCLOCKS;++i) {
		outbit=grain_keystream(mygrain);
		mygrain->LFSR[79]^=outbit;
		mygrain->NFSR[79]^=outbit;             
	}

}


int main() {
    grain mygrain;
    int IV[SAMPLE_SIZE][8];
    int keystream2[SAMPLE_SIZE][10];
    int i;

    int key1[10] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x12, 0x34};

    // Set the random seed using the current time
    srand(time(NULL));

    for (i = 0; i < SAMPLE_SIZE; ++i) {
        // Generate random IV
        int IV2[8];
        for (int j = 0; j < 8; ++j) {
            IV2[j] = rand() % 256; // Random number between 0 and 255
        }

        keysetup(&mygrain, key1, 80, 64);
        ivsetup(&mygrain, IV2);
        keystream_bytes(&mygrain, keystream2[i], 10);

        // Copy IV to the array
        for (int j = 0; j < 8; ++j) {
            IV[i][j] = IV2[j];
        }
    }

    // Print the first 10 samples
    for (i = 0; i < 10; ++i) {
        printData(IV[i], keystream2[i]);
    }

    int numSamples = SAMPLE_SIZE;

    // Call the function to transfer the data to the CSV file
    transferDataToCSV(IV, keystream2, numSamples);

    return 0;
}