/*
 *  Spazio IT - Generic SNAP7 Client
 *
 *  Copyright (C) 2020 - Spazio IT - https://www.spazioit.com
 *
 * This SNAP7 Client is free software: you can redistribute it and/or modify
 * it under the terms of the Lesser GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * It means that you can distribute your commercial software derived from this
 * SNAP7 Client without the requirement to distribute the source code of your
 * application and without the requirement that your application be itself
 * distributed under LGPL.
 *
 * This SNAP7 Client is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Lesser GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License and a
 * copy of Lesser GNU General Public License along with this SNAP7 Client.
 * If not, see  http://www.gnu.org/licenses/
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "snap7.h"

#ifdef OS_WINDOWS
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

TS7Client * client;

int dBNum = 1000;
int start = 0;

char *address;     // PLC IP address
int rack=0,slot=2; // Default rack and slot

bool jobDone=false;
int jobResult=0;

/*
 * Generic Callback to be used in asynchronous call
 */ 
void S7API cliCompletion(void *usrPtr, int opCode, int opResult)
{
    jobResult=opResult;
    jobDone = true;
}

/*
 * Portable Sleep
 */ 
void sysSleep(longword Delay_ms)
{
#ifdef OS_WINDOWS
	Sleep(Delay_ms);
#else
    struct timespec ts;
    ts.tv_sec = (time_t)(Delay_ms / 1000);
    ts.tv_nsec =(long)((Delay_ms - ts.tv_sec) * 1000000);
    nanosleep(&ts, (struct timespec *)0);
#endif
}

#define STR_LEN 50
uint8_t  boolValue = 0;
int16_t  intValue = 0;
int32_t  dintValue = 0;
uint8_t  strValue[STR_LEN];
uint8_t  strBuffer[STR_LEN+2];

#define WRONG_ARGUMENTS_COUNT               -1
#define WRONG_OPERATION                     -2
#define WRONG_ARGUMENTS_COUNT_FOR_OPERATION -3
#define WRONG_DBNUMBER                      -4
#define WRONG_START                         -5
#define WRONG_TYPE                          -6
#define WRONG_STR50_LENGTH                  -7
#define WRONG_CONSOLE_READ                  -8
#define WRONG_CONNECTION                    -9

#define MY_SELF "si-snap7-client"

/*
 * Swap functions to handle endianess
 */
void swapBinary(uint16_t &value) {
    value = (value >> 8) | (value << 8);
}
void swapBinary(uint32_t &value) {
    std::uint32_t tmp = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
    value = (tmp << 16) | (tmp >> 16);
}

/*
 * Print usage information
 */
void usage(const char * error)
{
    if (strcmp(error, "") != 0) {
        fprintf(stderr, "Error: %s!\n", error);
    }
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s <ipaddress> -r|-w <dbnumber> <start> <type> [<value>]\n", MY_SELF);
    fprintf(stderr, "valid types are: BOOL, INT, DINT, STR50 (string of max 50 chars);\n");
    fprintf(stderr, "example:\n");
    fprintf(stderr, "  %s 10.10.100.125 -r  1000 0 STR50\n", MY_SELF);
    fprintf(stderr, "or:\n");
    fprintf(stderr, "  %s 10.10.100.125 -w  1001 0 STR50 \"Fred\"\n", MY_SELF);
}

/*
 * Print error information
 */
void printError(const char * error)
{
    if (strcmp(error, "") != 0) {
        fprintf(stderr, "Error: %s!\n", error);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 6 && argc != 7) {
        usage("WRONG_ARGUMENTS_COUNT");
        return WRONG_ARGUMENTS_COUNT;
    }
    if (strcmp(argv[2], "-r") != 0 && strcmp(argv[2], "-w") != 0) {
        usage("WRONG_OPERATION");
        return WRONG_OPERATION;
    }
    if (argc == 6 && strcmp(argv[2], "-r") != 0) {
        usage("WRONG_ARGUMENTS_COUNT_FOR_OPERATION");
        return WRONG_ARGUMENTS_COUNT_FOR_OPERATION;
    }
    if (argc == 7 && strcmp(argv[2], "-w") != 0) {
        usage("WRONG_ARGUMENTS_COUNT_FOR_OPERATION");
        return WRONG_ARGUMENTS_COUNT_FOR_OPERATION;
    }
    if (sscanf_s(argv[3], "%d", &dBNum) != 1) {
        usage("WRONG_DBNUMBER");
        return WRONG_DBNUMBER;
    }
    if (sscanf_s(argv[4], "%d", &start) != 1) {
        usage("WRONG_START");
        return WRONG_START;
    }
    if (strcmp(argv[5], "BOOL") != 0 &&
        strcmp(argv[5], "INT") != 0 && 
        strcmp(argv[5], "DINT") != 0 && 
        strcmp(argv[5], "STR50") != 0) {
        usage("WRONG_TYPE");
        return WRONG_TYPE;
    }
    if (argc == 7 && strcmp(argv[2], "-w") == 0 && strlen(argv[6]) > STR_LEN) {
        usage("WRONG_STR50_LENGTH");
        return WRONG_STR50_LENGTH;
    }

    address = argv[1];

    client= new TS7Client();
    client->SetAsCallback(cliCompletion,NULL);

    /*
     * Please note that the port is embedded as #define in the SNAP7 library (.dll).
     * This is why there are two different versions of the library:
     * one with port 102 and the other with port 103
     */
    int ret_val = 0;
    int res = client->ConnectTo(address,rack,slot);
    if (res == 0) {
        if (strcmp(argv[2], "-r") == 0) {
            if (strcmp(argv[5], "BOOL") == 0) {
                client->DBRead(dBNum, start, 1, (void *) &boolValue);
                printf("%hhu\n", boolValue);
                fprintf(stderr, "READ: %hhu\n", boolValue);
            } else if (strcmp(argv[5], "INT") == 0) {
                client->DBRead(dBNum, start, 2, (void *) &intValue);
                swapBinary((uint16_t &) intValue);
                printf("%d\n", intValue);
                fprintf(stderr, "READ: %hd\n", intValue);
            } else if (strcmp(argv[5], "DINT") == 0) {
                client->DBRead(dBNum, start, 4, (void *) &dintValue);
                swapBinary((uint32_t &) dintValue);
                printf("%d\n", dintValue);
                fprintf(stderr, "READ: %d\n", dintValue);
            } else  {
                client->DBRead(dBNum, start, STR_LEN+2, strBuffer);
                int len = min(strBuffer[1], STR_LEN);
                int i;
                for (i = 0; i < len; i++) {
                    strValue[i] = strBuffer[i+2];
                }
                if (i < STR_LEN) {
                    strValue[i] = 0;
                }
                printf("%s\n", strValue);
                fprintf(stderr, "READ: %s\n", strValue);
            }
        } else {
            if (strcmp(argv[5], "BOOL") == 0) {
                if (sscanf_s(argv[6], "%hhu", &boolValue) == 1) {
                    client->DBWrite(dBNum, start, 1, (void *) &boolValue);
                    fprintf(stderr, "WROTE: %hhu\n", boolValue);
                } else {
                    usage("WRONG_CONSOLE_READ");
                    ret_val = WRONG_CONSOLE_READ;
                }
            } else if (strcmp(argv[5], "INT") == 0) {
                if (sscanf_s(argv[6], "%hd", &intValue) == 1) {
                    swapBinary((uint16_t &) intValue);
                    client->DBWrite(dBNum, start, 2, (void *) &intValue);
                    swapBinary((uint16_t &) intValue);
                    fprintf(stderr, "WROTE: %hd\n", intValue);
                } else {
                    usage("WRONG_CONSOLE_READ");
                    ret_val = WRONG_CONSOLE_READ;
                }
            } else if (strcmp(argv[5], "DINT") == 0) {
                if (sscanf_s(argv[6], "%d", &dintValue) == 1) {
                    swapBinary((uint32_t &) dintValue);
                    client->DBWrite(dBNum, start, 4, (void *) &dintValue);
                    swapBinary((uint32_t &) dintValue);
                    fprintf(stderr, "WROTE: %d\n", dintValue);
                } else {
                    usage("WRONG_CONSOLE_READ");
                    ret_val = WRONG_CONSOLE_READ;
                }
            } else  {
                strncpy((char *) strValue, argv[6], STR_LEN);
                strValue[strlen(argv[6])] = '\0';
                strBuffer[0] = STR_LEN;
                strBuffer[1] = strlen((char *) strValue);
                for (int i = 0; i < strBuffer[i]; i++) {
                    strBuffer[i+2] = strValue[i];
                }
                client->DBWrite(dBNum, start, STR_LEN+2, (void *) strBuffer);
                fprintf(stderr, "WROTE: %s\n", strValue);
            }
        }
        client->Disconnect();
    } else {
        printError("WRONG_CONNECTION");
        ret_val = WRONG_CONNECTION;
    }
    return ret_val;
}

