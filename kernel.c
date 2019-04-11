#define MAX_BYTE 256
#define SECTOR_SIZE 512
#define MAX_DIRS 32
#define MAX_FILES 32
#define MAX_DIRNAME 15
#define MAX_FILENAME 15
#define MAX_SECTORS 16
#define DIR_ENTRY_LENGTH 32
#define MAP_SECTOR 256
#define DIRS_SECTOR 257
#define FILES_SECTOR 258
#define SECTORS_SECTOR 259
#define ARGS_SECTOR 512
#define TRUE 1
#define FALSE 0
#define INSUFFICIENT_ENTRIES -3
#define ALREADY_EXISTS -2
#define INSUFFICIENT_SECTORS 0
#define NOT_FOUND -1
#define INSUFFICIENT_DIR_ENTRIES -1
#define EMPTY 0x00
#define USED 0xFF

int mod (int a, int b);
int div (int a, int b);
void handleInterrupt21 (int AX, int BX, int CX, int DX);
void printString(char *string);
void readString(char *string);
void readSector(char *buffer, int sector);
void writeSector(char *buffer, int sector);
void readFile(char *buffer, char *path, int *result, char parentIndex);
void clear(char *buffer, int length);
void writeFile(char *buffer, char *path, int *sectors, char parentIndex);
void executeProgram(char *path, int segment, int *result, char parentIndex);
void makeDirectory(char *path, int *result, char parentIndex);
void writeFile(char *buffer, char *path, int *sectors, char parentIndex);
void deleteFile(char *path, int *result, char parentIndex);
void deleteDirectory(char *path, int *success, char parentIndex);
void terminateProgram (int *result);
void searchDir(char *path, int *success, char *parentIndex, int *idx);
void deleteFileFromSector(char parentIndex);
void putArgs (char curdir, char argc, char **argv);
void getCurdir (char *curdir);
void getArgc (char *argc);
void getArgv (char index, char *argv);
int equalPath(char* p1, char* p2);

int main() {
    char buffer[SECTOR_SIZE*MAX_SECTORS];
    makeInterrupt21();
    interrupt(0x21, 0x00, "milOS v0.69\r\n", 0, 0, 0);
    interrupt(0x21, 0x07, 0, 0, 0);
    
    while (1);
}


int mod(int a, int b) {
    while(a >= b) {
        a = a - b;
    }
    return a;
}

int div(int a, int b) {
    int q = 0;
    while(q*b <= a) {
        q = q+1;
    }
    return q-1;
}

void handleInterrupt21 (int AX, int BX, int CX, int DX)
{/* condition/* condition */
    char AL, AH;
    AL = (char) (AX);
    AH = (char) (AX >> 8);
    
    switch (AL) {
        case 0x00:
            printString(BX);
            break;
        case 0x01:
            readString(BX);
            break;
        case 0x02:
            readSector(BX, CX);
            break;
        case 0x03:
            writeSector(BX, CX);
            break;
        case 0x04:
            readFile(BX, CX, DX, AH);
            break;
        case 0x05:
            writeFile(BX, CX, DX, AH);
            break;
        case 0x06:
            executeProgram(BX, CX, DX, AH);
            break;
        case 0x07:
            terminateProgram(BX);
            break;
        case 0x08:
            makeDirectory(BX, CX, AH);
            break;
        case 0x09:
            deleteFile(BX, CX, AH);
            break;
        case 0x0A:
            deleteDirectory(BX, CX, AH);
            break;
        case 0x20:
            putArgs(BX, CX, DX);
            break;
        case 0x21:
            getCurdir(BX);
            break;
        case 0x22:
            getArgc(BX);
            break;
        case 0X23:
            getArgv(BX, CX);
            break;
        default:
            printString("Invalid interrupt");
    }
}

void printString(char *string)
{
    int i;
    
    i = 0;
    while (string[i] != '\0')
    {
        interrupt(0x10, 0xE00 + string[i], 0, 0, 0);
        i++;
    }
}

void readString(char *string)
{
    int i = 0;
    
    string[i] = interrupt(0x16, 0, 0, 0, 0);
    while (string[i] != '\r')
    {
        interrupt(0x10, 0xE00 + string[i], 0, 0, 0);
        if (string[i] == '\b')
        {
            interrupt(0x10, 0xE00 + '\0', 0, 0, 0);
            interrupt(0x10, 0xE00 + '\b', 0, 0, 0);
            i -= 2;
        }
        i++;
        string[i] = interrupt(0x16, 0, 0, 0, 0);
    }
    string[i] = '\0';
    interrupt(0x10, 0xE00 + '\n', 0, 0, 0);
    interrupt(0x10, 0xE00 + '\r', 0, 0, 0);
}

void readSector(char *buffer, int sector)
{
    interrupt(0x13, 0x201, buffer, div(sector, 36) * 0x100 + mod(sector, 18) + 1, mod(div(sector, 18), 2) * 0x100);
}

void writeSector(char *buffer, int sector)
{
    interrupt(0x13, 0x301, buffer, div(sector, 36) * 0x100 + mod(sector, 18) + 1, mod(div(sector, 18), 2) * 0x100);
}

void clear(char *buffer, int length) {
    int i;
    for(i = 0; i < length; ++i) {
        buffer[i] = EMPTY;
    }
}

void executeProgram(char *path, int segment, int *result, char parentIndex)
{
    char buffer[MAX_SECTORS*SECTOR_SIZE];
    int success;
    int i;

    readFile(buffer, path, &success, parentIndex);
    if (success == 0)
    {
        for (i = 0; i < MAX_SECTORS*SECTOR_SIZE; i++)
        {
            putInMemory(segment, i, buffer[i]);
        }
        launchProgram(segment);
        *result = TRUE;
    }
    else
    {
        printString("failed");
        *result = FALSE;
    }
    
}

void readFile(char *buffer, char *path, int *result, char parentIndex)
{
    char files[SECTOR_SIZE];
    char sectors[SECTOR_SIZE];
    char currentPath[15];
    int i, j, k, fileEqual;
    
    searchDir(path, result, &parentIndex, &i);
    
    if (*result == FALSE)
    {
        *result = NOT_FOUND;
        return;
    }
    
    readSector(files, FILES_SECTOR);
    k = 0;
    clear(currentPath, MAX_DIRNAME);
    while (k < MAX_FILENAME && path[i] != '\0')
    {
        currentPath[k] = path[i];
        k++;
        i++;
    }
    j = 0;
    while (j*16 < SECTOR_SIZE)
    {
        fileEqual = equalPath(currentPath, files+j*16+1);
        if (fileEqual == TRUE && files[j*16] == parentIndex)
        {
            parentIndex = j;
            break;
        }
        else
        {
            j++;
        }
    }
    if (j == 16)
    {
        *result = NOT_FOUND;
        return;
    }
    
    readSector(sectors, SECTORS_SECTOR);
    clear(buffer, MAX_SECTORS);
    i = 0;
    do
    {
        readSector(buffer+i*SECTOR_SIZE, sectors[i+parentIndex*MAX_SECTORS]);
        i++;
    } while (i < MAX_SECTORS && sectors[i+parentIndex*MAX_SECTORS] != '\0');
    *result = 0;
}

void writeFile(char *buffer, char *path, int *sectors, char parentIndex)
{
    char map[SECTOR_SIZE];
    char files[SECTOR_SIZE];
    int i, j;
    
    readSector(map, MAP_SECTOR);
    
    
}

void makeDirectory(char *path, int *result, char parentIndex)
{
    char dirs[SECTOR_SIZE];
    char files[SECTOR_SIZE];
    char currentPath[MAX_DIRNAME];
    int i, j, k, l, fileEqual;
    
    clear(dirs, SECTOR_SIZE);
    readSector(dirs, DIRS_SECTOR);
    i = 0;
    while (i < 32 && dirs[i*16+1] != '\0')
    {
        i++;
    }
    if (i == 32)
    {
        *result = INSUFFICIENT_ENTRIES;
        return;
    }
    searchDir(path, result, &parentIndex, &j);
    if (*result == FALSE)
    {
        *result = NOT_FOUND;
        return;
    }
    clear(files, SECTOR_SIZE);
    readSector(files, FILES_SECTOR);
    l = 0;
    j++;
    clear(currentPath, MAX_DIRNAME);
    while (l < MAX_FILENAME && path[j] != '\0')
    {
        currentPath[l] = path[j];
        l++;
        j++;
    }
    k = 0;
    while (k*16 < SECTOR_SIZE)
    {
        fileEqual = equalPath(currentPath, files+k*16+1);
        if (fileEqual == TRUE && files[j*16] == parentIndex)
        {
            *result = ALREADY_EXISTS;
            return;
        }
        else
        {
            k++;
        }
    }
    if (k == 16)
        dirs[i*16] = parentIndex;
    l = 1;
    while (l < 16 && currentPath[l-1] != '\0')
    {
        dirs[i*16 + l] = currentPath[l-1];
        l++;
    }
    writeSector(dirs, DIRS_SECTOR);
    *result = 0;
    return;
}

int equalPath(char* p1, char* p2)
{
    int i = 0;
    while (p1[i] == p2[i] && i < MAX_DIRNAME)
    {
        i++;
    }
    if(p1[i] != p2[i])
    {
        return FALSE;
    }
    else if (p1[i] == '\0' && p2[i] == '\0' && i != 0)
    {
        return TRUE;
    }
    return TRUE;
}
void deleteFile(char *path, int *result, char parentIndex)
{
    char files[SECTOR_SIZE];
    char sectors[SECTOR_SIZE];
    char map[MAP_SECTOR];
    char currentPath[15];
    int i, j, k, fileEqual;
    
    searchDir(path, result, &parentIndex, &i);
    if (*result == FALSE)
    {
        *result = NOT_FOUND;
        return;
    }
    
    i++;
    j = 0;
    while (j < MAX_FILENAME && path[i] != '\0')
    {
        currentPath[j] = path[i];
    }
    readSector(files, FILES_SECTOR);
    k = 0;
    while (k < 32)
    {
        fileEqual = equalPath(currentPath, files+k*16+1);
        if (files[k*16] == parentIndex && fileEqual == TRUE)
        {
            files[k*16+1] = '\0';
            parentIndex = k;
            break;
        }
        else
        {
            j++;
        }
    }
    if (k == 32)
    {
        *result = NOT_FOUND;
        return;
    }
    
    clear(files+parentIndex*16, 16);
    
    readSector(sectors, SECTORS_SECTOR);
    readSector(map, MAP_SECTOR);
               
    i = 0;
    do
    {
       map[sectors[i+parentIndex*MAX_SECTORS]] = '\0';
       i++;
    } while (i < MAX_SECTORS && sectors[i+parentIndex*MAX_SECTORS] != '\0');
   
    clear(sectors+parentIndex*16, 16);
    *result = 0;
   
   writeSector(map, MAP_SECTOR);
   writeSector(files, FILES_SECTOR);
   writeSector(sectors, SECTORS_SECTOR);
}
               
void deleteDirectory(char *path, int *success, char parentIndex)
{
    char dirs[SECTOR_SIZE];
    char files[SECTOR_SIZE];
    char currentPath[15];
    int i, j,fileEqual;
    
    searchDir(path, success, &parentIndex, &i);
    if (*success == FALSE)
    {
        *success = NOT_FOUND;
        return;
    }
    
    i++;
    j = 0;
    while (path[i] != '\0' && j < MAX_FILENAME)
    {
        currentPath[j] = path[i];
        i++;
        j++;
    }
    
    j = 0;
    readSector(dirs, DIRS_SECTOR);
    while (j < 32)
    {
        fileEqual = equalPath(currentPath, dirs+j*16+1);
        if (fileEqual == TRUE && dirs[j*16] == parentIndex)
            break;
        j++;
    }
    if (j == 32)
    {
        *success = NOT_FOUND;
        return;
    }
    
    dirs[j*16+1] = '\0';
    deleteFileFromSector(parentIndex);
    /*Belum selesai*/
    
}
               
void searchDir(char *path, int *success, char *parentIndex, int *idx)
{
    char dirs[SECTOR_SIZE];
    char currentPath[15];
    int i,j,k,dirEqual;
    
    i = 0;
    while (i < MAX_FILENAME && path[i] != '/' && path[i] != '\0')
    {
        currentPath[i] = path[i];
        i++;
    }
    if(path[i] == '\0')
    {
        *success = 5;
        return;
    }
    j = 0;
    clear(dirs, SECTOR_SIZE);
    readSector(dirs, DIRS_SECTOR);
    while (j < 32)
    {
        dirEqual = equalPath(currentPath, dirs+j*16+1);
        if (dirs[j*16] == *parentIndex && dirEqual == TRUE)
        {
            *parentIndex = j;
            break;
        }
        else
        {
            j++;
        }
    }
    if (j == 32)
    {
        *success = FALSE;
        return;
    }
    
    k = 0;
    i++;
    clear(currentPath, MAX_DIRNAME);
    while (k < MAX_DIRNAME && path[i] != '/')
    {
        currentPath[k] = path[i];
        i++;
    }
    *idx = i;
    j = 0;
    while (j < 32)
    {
        dirEqual = equalPath(currentPath, dirs+j*16+1);
        if (dirs[j*16] == *parentIndex && dirEqual == TRUE)
        {
            *parentIndex = j;
            break;
        }
        else
        {
            j++;
        }
    }
    if (j == 32)
    {
        *success = FALSE;
        return;
    }
}
               
void putArgs (char curdir, char argc, char **argv)
{
   char args[SECTOR_SIZE];
   int i, j, p;
   clear(args, SECTOR_SIZE);
   
   args[0] = curdir;
   args[1] = argc;
   i = 0;
   j = 0;
   for (p = 2; p < ARGS_SECTOR && i < argc; ++p) {
       args[p] = argv[i][j];
       if (argv[i][j] == '\0') {
           ++i;
           j = 0;
       }
       else {
           ++j;
       }
   }
   
   writeSector(args, ARGS_SECTOR);
}
               
void getCurdir (char *curdir)
{
    char args[SECTOR_SIZE];
    readSector(args, ARGS_SECTOR);
    *curdir = args[0];
}
               
void getArgc (char *argc)
{
   char args[SECTOR_SIZE];
   readSector(args, ARGS_SECTOR);
   *argc = args[1];
}

void getArgv (char index, char *argv) {
   char args[SECTOR_SIZE];
   int i, j, p;
   readSector(args, ARGS_SECTOR);
   
   i = 0;
   j = 0;
   for (p = 2; p < ARGS_SECTOR; ++p) {
       if (i == index) {
           argv[j] = args[p];
           ++j;
       }
       /* code */
       if (args[p] == '\0') {
           if (i == index) {
               break;
           }
           else {
               ++i;
           }
       }
   }
}

void terminateProgram (int *result) {
   char shell[6];
   shell[0] = 's';
   shell[1] = 'h';
   shell[2] = 'e';
   shell[3] = 'l';
   shell[4] = 'l';
   shell[5] = '\0';
   executeProgram(shell, 0x2000, result, 0xFF);
}


void deleteFileFromSector(char parentIndex)
{
    char files[SECTOR_SIZE];
    char sectors[SECTORS_SECTOR];
    char map[SECTOR_SIZE];
    int i, j;
    
    readSector(files, FILES_SECTOR);
    readSector(sectors, SECTORS_SECTOR);
    readSector(map, MAP_SECTOR);
    clear(files+parentIndex*16, 16);
    
    for (j = 0; j < 32; j++)
    {
        if (files[j*16] == parentIndex)
        {
            files[j*16+1] = '\0';
            i = 0;
            do
            {
                map[sectors[i+j*MAX_SECTORS]] = '\0';
                i++;
            } while (i < MAX_SECTORS && sectors[i+j*MAX_SECTORS] != '\0');
            
            clear(sectors+j*16, 16);
        }
    }
    
    writeSector(map, MAP_SECTOR);
    writeSector(files, FILES_SECTOR);
    writeSector(sectors, SECTORS_SECTOR);
}
