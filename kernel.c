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
void readFile(char *buffer, char *filename, int *success);
void clear(char *buffer, int length);
void writeFile(char *buffer, char *filename, int *sectors);
void executeProgram(char *filename, int segment, int *success);
/*Yang baru*/
void executeProgram(char *path, int segment, int *result, char parentIndex);
void writeFile(char *buffer, char *path, int *sectors, char parentIndex);
void makeDirectory(char *path, int *result, char parentIndex);
void terminateProgram (int *result);
void deleteFile(char *path, int *result, char parentIndex);
void deleteDirectory(char *path, int *success, char parentIndex);


int main() {
   char buffer[MAX_SECTORS*SECTOR_SIZE];

   makeInterrupt21();
   handleInterrupt21(0x1, 0, 0, 0);
   handleInterrupt21(0x0, 0, 0, 0);
   handleInterrupt21(0x6, "keyproc", 0x2000, 0);

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

void handleInterrupt21 (int AX, int BX, int CX, int DX) {
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
         putArgs(BX, CX);
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

void readFile(char *buffer, char *filename, int *success)
{
  char dir[SECTOR_SIZE];
  int i, n, j;

  *success = FALSE;
  i = 0;
  n = 0;
  readSector(dir, DIR_SECTOR);
  while (i < 16 && *success == FALSE)
  {
    while (n < MAX_FILENAME && dir[n + 32*i] == filename[n])
    {
      n++;
    }
    if (n < MAX_FILENAME)
    {
      i++;
    }
    else if (n == MAX_FILENAME)
    {
      *success = TRUE;
    }
  }

  if (i == 16)
  {
    *success = FALSE;
  }
  else
  {
    j = 0;
    while (j < MAX_SECTORS && dir[n + 32*i] != 0x00)
    {
      readSector(buffer + j*SECTOR_SIZE, dir[n + 32*i]);
      n++;
      j++;
    }
  }
}

void clear(char *buffer, int length) {
   int i;
   for(i = 0; i < length; ++i) {
      buffer[i] = EMPTY;
   }
}

void writeFile(char *buffer, char *filename, int *sectors)
{
  char map[SECTOR_SIZE];
  char dir[SECTOR_SIZE];
  char sectorBuffer[SECTOR_SIZE];
  int dirIndex;

  readSector(map, MAP_SECTOR);
  readSector(dir, DIR_SECTOR);

  for (dirIndex = 0; dirIndex < MAX_FILES; ++dirIndex) {
     if (dir[dirIndex * DIR_ENTRY_LENGTH] == '\0') {
        break;
     }
  }

  if (dirIndex < MAX_FILES)
  {
     int i, j, sectorCount;
     for (i = 0, sectorCount = 0; i < MAX_BYTE && sectorCount < *sectors; ++i) {
        if (map[i] == EMPTY) {
           ++sectorCount;
        }
     }

     if (sectorCount < *sectors) {
        *sectors = INSUFFICIENT_SECTORS;
        return;
     } else {
        clear(dir + dirIndex * DIR_ENTRY_LENGTH, DIR_ENTRY_LENGTH);
        for (i = 0; i < MAX_FILENAME; ++i) {
           if (filename[i] != '\0') {
              dir[dirIndex * DIR_ENTRY_LENGTH + i] = filename[i];
           } else {
              break;
           }
        }

        for (i = 0, sectorCount = 0; i < MAX_BYTE && sectorCount < *sectors; ++i) {
           if (map[i] == EMPTY) {
              map[i] = USED;
              dir[dirIndex * DIR_ENTRY_LENGTH + MAX_FILENAME + sectorCount] = i;
              clear(sectorBuffer, SECTOR_SIZE);
              for (j = 0; j < SECTOR_SIZE; ++j) {
                 sectorBuffer[j] = buffer[sectorCount * SECTOR_SIZE + j];
              }
              writeSector(sectorBuffer, i);
              ++sectorCount;
           }
        }
     }
  } else {
     *sectors = INSUFFICIENT_DIR_ENTRIES;
     return;
  }

  writeSector(map, MAP_SECTOR);
  writeSector(dir, DIR_SECTOR);
}



void executeProgram(char *filename, int segment, int *success)
{
  char buffer[MAX_SECTORS*SECTOR_SIZE];
  int i;

  readFile(buffer, filename, success);
  if (*success == TRUE)
  {
    for (i = 0; i < (MAX_SECTORS*SECTOR_SIZE); i++)
    {
      putInMemory(segment, i, buffer[i]);
    }
    launchProgram(segment);
  }
}

void executeProgram(char *path, int segment, int *result, char parentIndex)
{
    char buffer[MAX_SECTORS*SECTOR_SIZE];
    int i;
    
    readFile(buffer, path, result,parentIndex);
    if (*result == TRUE)
    {
        for (i = 0; i < (MAX_SECTORS*SECTOR_SIZE); i++)
        {
            putInMemory(segment, i, buffer[i]);
        }
        launchProgram(segment);
    }
}

void readFile(char *buffer, char *path, int *result, char parentIndex)
{
  char dirs[SECTOR_SIZE];
  char files[SECTOR_SIZE];
  char sectors[SECTOR_SIZE];

}

void writeFile(char *buffer, char *path, int *sectors, char parentIndex)
{
    char map[SECTOR_SIZE];
    char dirs[SECTOR_SIZE];
    char files[SECTOR_SIZE];
    char sectors[SECTOR_SIZE];
    char checkDir[MAX_FILENAME];
    char tempBuff[SECTOR_SIZE];
    char currParIdx;
    int i, j, sectCount, cukup, row, col, idxPath,found, bufferIdx, dirFound, arrLen, fileIdx;
    
    clear(map,SECTOR_SIZE);
    readSector(map, MAP_SECTOR);
    found = FALSE;
    i = 0;
    while (!found && i<SECTOR_SIZE)
    {
        found = map[i] == '\0';
    }
    if(!found)
    {
        *sectors = INSUFFICIENT_SECTORS;
    }
    else
    {
        clear(files,SECTOR_SIZE);
        readSector(files,FILES_SECTOR);
        clear(dirs,SECTOR_SIZE);
        readSector(dirs,DIRS_SECTOR);
        i = 1;
        cukup = FALSE;
        while(!cukup && i<SECTOR_SIZE)
        {
            if(files[i] == '\0' )
            {
                cukup = TRUE;
            }
            else
            {
                i += 16;
            }
        }
        if(!cukup)
        {
            *result = INSUFFICIENT_ENTRIES;
        }
        else
        {
            currParIdx = parentIndex;
            row = 0;
            idxPath = 0;
            i = 0;
            dirFound = TRUE;
            clear(checkDir, MAX_FILENAME);
            while (path[idxPath] != '\0' && dirFound)
            {
                if(path[idxPath] != '/')
                {
                    checkDir[i] = path[idxPath];
                    i++;
                    idxPath++;
                }
                else        //Menemukan '/'
                {
                    found = FALSE;
                    while (row<SECTOR_SIZE && !found)
                    {
                        col = row;
                        if(currParIdx == dirs[col])
                        {
                            col++;
                            i = 0;
                            while (i < MAX_FILENAME - 1 && dirs[col] == checkDir[i] && dirs[col] != '\0' && checkDir[i] != '\0')
                            {
                                col++;
                                i++;
                            }
                            if(dirs[col] == '\0' || checkDir[i] == '\0')
                            {
                                if(dirs[col] == '\0' && checkDir[i] == '\0')        //found
                                {
                                    currParIdx = row;
                                    found = TRUE;
                                }
                                else
                                {
                                    row = (row+1)*16;
                                }
                            }
                            else if(dirs[col] == checkDir[i])       //kasus extreme ketika panjang dirName = 15
                            {
                                currParIdx = row;
                                found = TRUE;
                            }
                            else        //Tidak ditemukan
                            {
                                row = (row+1)*16;
                            }
                        }
                        else
                        {
                            row = (row+1)*16;
                        }
                    }
                    if(found)
                    {
                        i = 0;
                        idxPath++;
                    }
                    else
                    {
                        dirFound = FALSE;
                    }
                }
            }   //Keluar while dengan keadaan akhir string path atau suatu directory tidak ditemukan
            if(!dirFound)       //Suatu directory parent tidak ditemukan
            {
                *result = NOT_FOUND;
            }
            else
            {
                while (row<SECTOR_SIZE && !found)       //Untuk memeriksa apakah dir yang akan dibuat sudah ada
                {
                    col = row;
                    if(currParIdx == files[col])
                    {
                        col++;
                        i = 0;
                        while (i < MAX_FILENAME - 1 && files[col] == checkDir[i] && files[col] != '\0' && checkDir[i] != '\0')
                        {
                            col++;
                            i++;
                        }
                        if(files[col] == '\0' || checkDir[i] == '\0')
                        {
                            if(files[col] == '\0' && checkDir[i] == '\0')        //found
                            {
                                found = TRUE;
                            }
                            else
                            {
                                row = (row+1)*16;
                            }
                        }
                        else if(files[col] == checkDir[i])       //kasus extreme ketika panjang dirName = 15
                        {
                            found = TRUE;
                        }
                        else        //Tidak ditemukan
                        {
                            row = (row+1)*16;
                        }
                    }
                    else
                    {
                        row = (row+1)*16;
                    }
                }
                if(found)
                {
                    *result = ALREADY_EXISTS;
                }
                else
                {
                    row = 0;        //Digunakan untuk files
                    i = 0;          //Digunakan untuk map
                    sectCount = 0;  //Digunakan untuk berapa banyak sector yang dipakai file
                    bufferIdx = 0;  //Digunakan untuk mengiterasi isi buffer
                    found = FALSE;
                    clear(sectors, SECTOR_SIZE);
                    readSector(sectors, SECTORS_SECTOR);
                    while (i < SECTOR_SIZE && row < SECTOR_SIZE && sectCount < *sectors)
                    {
                        if(files[row] == '\0' && !found)
                        {
                            fileIdx = row;
                            found = TRUE;
                            arrLen = sizeof(checkDir)/sizeof(char);
                            col = row;
                            files[col] = currParIdx;
                            j = 0;      //Digunakan untuk traversal di checkDir
                            col++;
                            while (j < arrLen - 1 && checkDir[j] != '\0')
                            {
                                files[col] = checkDir[i];
                                col++;
                                j++;
                            }
                            if(checkDir[j] != '\0')
                            {
                                files[col] = checkDir[j];
                            }
                        }
                        else if(!found)
                        {
                            row = (row+1)*16;
                        }
                        if(map[i] == '\0')
                        {
                            map[i] = USED;  //i adalah sector yang kosong dan akan diisi oleh isi file
                            j = 0;      //Digunakan untuk mengiterasi tempBuff
                            sectors[fileIdx+sectCount] = i;
                            clear(tempBuff,SECTOR_SIZE);
                            while (j<SECTOR_SIZE && buffer[bufferIdx] != '\0')
                            {
                                tempBuff[j] = buffer[bufferIdx];
                                j++;
                                bufferIdx++;
                            }
                            writeSector(tempBuff,i);
                            sectCount++;
                        }
                    }
                    writeSector(map,MAP_SECTOR);
                    writeSector(files,FILES_SECTOR);
                    writeSector(sectors,SECTORS_SECTOR);
                }
            }
        }
    }
}

void makeDirectory(char *path, int *result, char parentIndex)
{
    char dirs[SECTOR_SIZE];
    char checkDir[MAX_FILENAME];        //Bertingkah seperti sebuah temp dir yg menyimpan nama directory yang akan diproses
    char currParIdx;
    int i, cukup, row, col, idxPath,found, dirFound, arrLen;
    
    clear(checkDir, MAX_FILENAME);      //Mengisi checkDir dengan \0
    clear(dirs,SECTOR_SIZE);
    readSector(dirs, DIRS_SECTOR);
    i = 1;
    cukup = FALSE;
    while(!cukup && i<SECTOR_SIZE)
    {
        if(dirs[i] == '\0' )
        {
          cukup = TRUE;
        }
        else
        {
          i += 16;
        }
    }
    if(!cukup)
    {
        *result = INSUFFICIENT_ENTRIES;
    }
    else
    {
        currParIdx = parentIndex;
        row = 0;
        idxPath = 0;
        i = 0;
        dirFound = TRUE;
        while (path[idxPath] != '\0' && dirFound)
        {
            if(path[idxPath] != '/')
            {
               checkDir[i] = path[idxPath];
               i++;
               idxPath++;
            }
            else        //Menemukan '/'
            {
                found = FALSE;
                while (row<SECTOR_SIZE && !found)
                {
                    col = row;
                    if(currParIdx == dirs[col])
                    {
                        col++;
                        i = 0;
                        while (i < MAX_FILENAME - 1 && dirs[col] == checkDir[i] && dirs[col] != '\0' && checkDir[i] != '\0')
                        {
                            col++;
                            i++;
                        }
                        if(dirs[col] == '\0' || checkDir[i] == '\0')
                        {
                            if(dirs[col] == '\0' && checkDir[i] == '\0')        //found
                            {
                                currParIdx = row;
                                found = TRUE;
                            }
                            else
                            {
                                row = (row+1)*16;
                            }
                        }
                        else if(dirs[col] == checkDir[i])       //kasus extreme ketika panjang dirName = 15
                        {
                            currParIdx = row;
                            found = TRUE;
                        }
                        else        //Tidak ditemukan
                        {
                            row = (row+1)*16;
                        }
                    }
                    else
                    {
                        row = (row+1)*16;
                    }
                }
                if(found)
                {
                    i = 0;
                    idxPath++;
                }
                else
                {
                    dirFound = FALSE;
                }
            }
        }   //Keluar while dengan keadaan akhir string path atau suatu directory tidak ditemukan
        if(!dirFound)       //Suatu directory parent tidak ditemukan
        {
            *result = NOT_FOUND;
        }
        else    //Proses pembuatan directory dengan keadaan isi checkdir adalah nama directory yang ingin dibuat
        {
            found = FALSE;
            row = 0;
            while (row<SECTOR_SIZE && !found)       //Untuk memeriksa apakah dir yang akan dibuat sudah ada
            {
                col = row;
                if(currParIdx == dirs[col])
                {
                    col++;
                    i = 0;
                    while (i < MAX_FILENAME - 1 && dirs[col] == checkDir[i] && dirs[col] != '\0' && checkDir[i] != '\0')
                    {
                        col++;
                        i++;
                    }
                    if(dirs[col] == '\0' || checkDir[i] == '\0')
                    {
                        if(dirs[col] == '\0' && checkDir[i] == '\0')        //found
                        {
                            found = TRUE;
                        }
                        else
                        {
                            row = (row+1)*16;
                        }
                    }
                    else if(dirs[col] == checkDir[i])       //kasus extreme ketika panjang dirName = 15
                    {
                        found = TRUE;
                    }
                    else        //Tidak ditemukan
                    {
                        row = (row+1)*16;
                    }
                }
                else
                {
                    row = (row+1)*16;
                }
            }
            if(found)
            {
                *result = ALREADY_EXISTS;
            }
            else
            {
                row = 0;
                found = FALSE;
                while (row < SECTOR_SIZE && !found)
                {
                    if(dirs[row] == '\0')
                    {
                        found = TRUE;
                    }
                    else if(dirs[row] == currParIdx)
                    {
                        col = row;
                        col++;
                        found = dirs[col] == '\0';
                    }
                    else
                    {
                        row = (row+1)*16;
                    }
                }
                if(found)
                {
                    arrLen = sizeof(checkDir)/sizeof(char);
                    col = row;
                    dirs[col] = currParIdx;
                    i = 0;
                    col++;
                    while (i < arrLen - 1 && checkDir[i] != '\0')
                    {
                        dirs[col] = checkDir[i];
                        col++;
                        i++;
                    }
                    if(checkDir[i] != '\0')
                    {
                        dirs[col] = checkDir[i];
                        *result = 0;
                    }
                    writeSector(dirs,DIRS_SECTOR);
                }
            }
        }
    }
}

void terminateProgram (int *result)
{
    char shell[6];
    shell[0] = 's';
    shell[1] = 'h';
    shell[2] = 'e';
    shell[3] = 'l';
    shell[4] = 'l';
    shell[5] = '\0';
    executeProgram(shell, 0x2000, result, 0xFF);
}

void deleteFile(char *path, int *result, char parentIndex)
{
    char dirs[SECTOR_SIZE];
    char files[SECTOR_SIZE];
    char map[SECTOR_SIZE];
    char sectors[SECTOR_SIZE];
    char checkDir[MAX_FILENAME];        //Bertingkah seperti sebuah temp dir yg menyimpan nama directory yang akan diproses
    char currParIdx;
    int i, row, col, idxPath,found, dirFound, delRow;
    
    clear(checkDir, MAX_FILENAME);      //Mengisi checkDir dengan \0
    
    clear(dirs,SECTOR_SIZE);
    readSector(dirs, DIRS_SECTOR);
    
    clear(map,SECTOR_SIZE);
    readSector(map, MAP_SECTOR);
    
    clear(files,SECTOR_SIZE);
    readSector(files, FILES_SECTOR);
    
    clear(sectors, SECTOR_SIZE);
    readSector(sectors, FILES_SECTOR);
    
    currParIdx = parentIndex;
    row = 0;
    idxPath = 0;
    i = 0;
    dirFound = TRUE;
    
    while (path[idxPath] != '\0' && dirFound)
    {
        if(path[idxPath] != '/')
        {
            checkDir[i] = path[idxPath];
            i++;
            idxPath++;
        }
        else        //Menemukan '/'
        {
            found = FALSE;
            while (row<SECTOR_SIZE && !found)
            {
                col = row;
                if(currParIdx == dirs[col])
                {
                    col++;
                    i = 0;
                    while (i < MAX_FILENAME - 1 && dirs[col] == checkDir[i] && dirs[col] != '\0' && checkDir[i] != '\0')
                    {
                        col++;
                        i++;
                    }
                    if(dirs[col] == '\0' || checkDir[i] == '\0')
                    {
                        if(dirs[col] == '\0' && checkDir[i] == '\0')        //found
                        {
                            currParIdx = row;
                            found = TRUE;
                        }
                        else
                        {
                            row = (row+1)*16;
                        }
                    }
                    else if(dirs[col] == checkDir[i])       //kasus extreme ketika panjang dirName = 15
                    {
                        currParIdx = row;
                        found = TRUE;
                    }
                    else        //Tidak ditemukan
                    {
                        row = (row+1)*16;
                    }
                }
                else
                {
                    row = (row+1)*16;
                }
            }
            if(found)
            {
                i = 0;
                idxPath++;
            }
            else
            {
                dirFound = FALSE;
            }
        }
    }   //Keluar while dengan keadaan akhir string path atau suatu directory tidak ditemukan
    if(!dirFound)       //Suatu directory parent tidak ditemukan
    {
        *result = NOT_FOUND;
    }
    else
    {
        row = 0;
        found = FALSE;
        while (row<SECTOR_SIZE && !found)
        {
            col = row;
            if(currParIdx == files[col])
            {
                col++;
                i = 0;
                while (i < MAX_FILENAME - 1 && files[col] == checkDir[i] && files[col] != '\0' && checkDir[i] != '\0')
                {
                    col++;
                    i++;
                }
                if(files[col] == '\0' || checkDir[i] == '\0')
                {
                    if(files[col] == '\0' && checkDir[i] == '\0')        //found
                    {
                        currParIdx = row;
                        found = TRUE;
                    }
                    else
                    {
                        row = (row+1)*16;
                    }
                }
                else if(files[col] == checkDir[i])       //kasus extreme ketika panjang dirName = 15
                {
                    currParIdx = row;
                    found = TRUE;
                }
                else        //Tidak ditemukan
                {
                    row = (row+1)*16;
                }
            }
            else
            {
                row = (row+1)*16;
            }
        }
        if(!found)
        {
            *result = NOT_FOUND;
        }
        else
        {
            delRow = row;
            files[delRow + 1] = '\0'
            i = 0;
            while (sectors[delRow + i] != '\0')
            {
                map[sectors[delRow+i]] = EMPTY;
                i++;
            }
            *result = 0;
            writeSector(map,MAP_SECTOR);
            writeSector(files, FILES_SECTOR);
        }
    }
}

void deleteDirectory(char *path, int *success, char parentIndex)
{
    char dirs[SECTOR_SIZE];
    int i, row, col, idxPath,found, dirFound;
    
    clear(checkDir, MAX_FILENAME);      //Mengisi checkDir dengan \0
    
    clear(dirs,SECTOR_SIZE);
    readSector(dirs, DIRS_SECTOR);
    
    currParIdx = parentIndex;
    row = 0;
    idxPath = 0;
    i = 0;
    dirFound = TRUE;
    
    while (path[idxPath] != '\0' && dirFound)
    {
        if(path[idxPath] != '/')
        {
            checkDir[i] = path[idxPath];
            i++;
            idxPath++;
        }
        else        //Menemukan '/'
        {
            found = FALSE;
            while (row<SECTOR_SIZE && !found)
            {
                col = row;
                if(currParIdx == dirs[col])
                {
                    col++;
                    i = 0;
                    while (i < MAX_FILENAME - 1 && dirs[col] == checkDir[i] && dirs[col] != '\0' && checkDir[i] != '\0')
                    {
                        col++;
                        i++;
                    }
                    if(dirs[col] == '\0' || checkDir[i] == '\0')
                    {
                        if(dirs[col] == '\0' && checkDir[i] == '\0')        //found
                        {
                            currParIdx = row;
                            found = TRUE;
                        }
                        else
                        {
                            row = (row+1)*16;
                        }
                    }
                    else if(dirs[col] == checkDir[i])       //kasus extreme ketika panjang dirName = 15
                    {
                        currParIdx = row;
                        found = TRUE;
                    }
                    else        //Tidak ditemukan
                    {
                        row = (row+1)*16;
                    }
                }
                else
                {
                    row = (row+1)*16;
                }
            }
            if(found)
            {
                i = 0;
                idxPath++;
            }
            else
            {
                dirFound = FALSE;
            }
        }
    }   //Keluar while dengan keadaan akhir string path atau suatu directory tidak ditemukan
    if(!dirFound)       //Suatu directory parent tidak ditemukan
    {
        *result = NOT_FOUND;
    }
    else
    {
        row = 0;
        found = FALSE;
        while (row<SECTOR_SIZE && !found)
        {
            col = row;
            if(currParIdx == files[col])
            {
                col++;
                i = 0;
                while (i < MAX_FILENAME - 1 && dirs[col] == checkDir[i] && dirs[col] != '\0' && checkDir[i] != '\0')
                {
                    col++;
                    i++;
                }
                if(dirs[col] == '\0' || checkDir[i] == '\0')
                {
                    if(dirs[col] == '\0' && checkDir[i] == '\0')        //found
                    {
                        currParIdx = row;
                        found = TRUE;
                    }
                    else
                    {
                        row = (row+1)*16;
                    }
                }
                else if(dirs[col] == checkDir[i])       //kasus extreme ketika panjang dirName = 15
                {
                    currParIdx = row;
                    found = TRUE;
                }
                else        //Tidak ditemukan
                {
                    row = (row+1)*16;
                }
            }
            else
            {
                row = (row+1)*16;
            }
        }
        if(!found)
        {
            *result = NOT_FOUND;
        }
        else
        {
            dirs[row+1] = '\0';
            
        }
    }
    
}

