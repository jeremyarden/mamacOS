#define SECTOR_SIZE 512
#define DIRS_SECTOR 257
#define FILES_SECTOR 258
#define EMPTY 0x00
#define MAX_FILENAME 15
void clear(char *buffer, int length);
void copyName(int idx,char *copy,char *target);
int main()
{
    char curDir;
    char dirs[SECTOR_SIZE];
    char files[SECTOR_SIZE];
    char currPath[MAX_FILENAME];
    int row,finished,processDir;
    row = 0;
    processDir = 0;
    finished = 0;
    clear(currPath, MAX_FILENAME);
    clear(dirs, SECTOR_SIZE);
    clear(files, SECTOR_SIZE);
    interrupt(0x21,0x21,&curDir,0,0);
    interrupt(0x21,0x02,dirs,DIRS_SECTOR,0);
    interrupt(0x21,0x02,files,FILES_SECTOR,0);
    while (!finished)
    {
        if(!processDir)
        {
            if(dirs[row] == curDir)
            {
                copyName(row+1, dirs, currPath);
                interrupt(0x21,0x01,currPath,0,0);
                clear(currPath, MAX_FILENAME);
                interrupt(0x21,0x01,"\r\n");
            }
            row+=16;
            if(row>=SECTOR_SIZE)
            {
                processDir = 1;
                row = 0;
            }
        }
        else
        {
            if(files[row] == curDir)
            {
                copyName(row+1, files, currPath);
                interrupt(0x21,0x01,currPath,0,0);
                clear(currPath, MAX_FILENAME);
                interrupt(0x21,0x01,"\r\n");
            }
            row+=16;
            if(row>=SECTOR_SIZE)
            {
                finished = 1;
            }
        }
    }
    
    return 0;
}

void clear(char *buffer, int length)
{
    int i;
    for(i = 0; i < length; ++i) {
        buffer[i] = EMPTY;
    }
}

void copyName(int idx,char *copy,char *target)
{
    while (copy[idx] != '\0')
    {
        target[idx] = copy[idx];
        idx++;
    }
}


