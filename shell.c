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

int stringlen(char* s);
int stringcmp(char* s1, char* s2);

int main() {
    char curdir = 0xFF; /* inisialisasi */
    char command[128];
    char buffer[512];
    char dirs[512];
    char argc;
    char argv[4][16];
    int result;

    //interrupt(0x21, 0x00, '$', 0x00, 0x00);
    while(1) {
        interrupt(0x21, 0x00, "$ ", 0x00, 0x00); /* print $ */

        interrupt(0x21, 0x01, command, 0x00, 0x00); /* nerima input char dengan nama variabel command */
        
        interrupt(0x21, 0x02, dirs, 257, 0x00); /* read sector dirs */
        
        /*if (command[0] ==  'c' && command[1] == 'd') { //implementasi cd
            if (command[2] == '.' && command[3] == '.') {
                if (curdir != 0xFF) {
                    // pindah ke parent dir
                    curdir = dirs[curdir*16];
                }
            }
            else if (command[2] == ' ') {
                if (command[3] == '.' && command[4] == '.') {
                    if (curdir != 0xFF) {
                        // pindah ke parent dir
                        curdir = dirs[curdir*16];
                    }
                }
                else {
                    // pindah ke child dir dengan nama yang sesuai inputan 
                    int i = 0;
                    int ketemu = FALSE;
                    while (i*16 <= 512 || !(ketemu)) {
                        if (dirs[i*16] == curdir) {
                            int j = i*16+1; // idx ngebaca dirs 
                            int k = 3; // idx ngebaca command 
                            while (j <= i*16+15 || (dirs[j] != '\0' && command[k] != '\0')) {
                                if (dirs[j] == command[k]) {
                                    j++;
                                    k++;
                                }
                                if ((j == i*16+15) && (command[k+1] == '\0')) { // kasus kalo dirs udh diujung dan command udh diujung juga 
                                    curdir = i;
                                    ketemu = TRUE;
                                }
                            }
                            if (dirs[j] == command[k])  {
                                curdir = i;
                                ketemu = TRUE;
                            }
                            else {
                                i++;
                            }
                        }
                        else {
                            i++;
                        }
                    }
                }
            }
        }*/
        if (stringcmp(command, "cd..") == TRUE || stringcmp(command, "cd ..") == TRUE) {
            if (curdir != 0xFF) {
                // pindah ke parent dir
                curdir = dirs[curdir*16];
            }
        }
        else { /* menjalankan program */
            if (command[0] == '.' && command[1] == '/') {
                int i = 2; /* indeks ngebaca command */
                int j = 0; /* indeks perintah */
                char *perintah;
                while (command[i] != ' ') {
                    perintah[j] = command[i];
                    j++;
                    i++;
                }
                i++; 
                argc = 0;
                while (command[i] != '\0') {
                    int k = 0; /* indeks argv[argc] */
                    while (command[i] != ' ') {
                        argv[argc][k] = command[i];
                        k++;
                        i++;
                    }
                    argc++;
                    i++;
                }
                interrupt(0x21, 0x20, curdir, argc, argv); /* put args */
                interrupt(0x21, 0x06, perintah, 0x2000, &result); /* execute program */
            }
            else {
                int i = 0; /* indeks ngebaca command */
                int j = 0; /* indeks perintah */
                char *perintah;
                while (command[i] != ' ') {
                    perintah[j] = command[i];
                    j++;
                    i++;
                }
                i++; 
                argc = 0;
                while (command[i] != '\0') {
                    int k = 0; /* indeks argv[argc] */
                    while (command[i] != ' ') {
                        argv[argc][k] = command[i];
                        k++;
                        i++;
                    }
                    argc++;
                    i++;
                }
                interrupt(0x21, 0x20, 0xFF, argc, argv); /* put args */
                interrupt(0x21, 0x06, perintah, 0x21000, &result); /* execute program */
            }
            if (result) {
                interrupt(0x21, 0x00, "Program executed", 0x00, 0x00);
            }
            else {
                interrupt(0x21, 0x00, "Execution error", 0x00, 0x00);
            }
        }
    }
}

int stringlen(char* s)
{
    int i = 0;

    while (s[i] != '\0')
    {
        i++;
    }

    return i;
}

int stringcmp(char* s1, char* s2)
{
    if (stringlen(s1) != stringlen(s2))
    {
        return FALSE;
    }
    else
    {
        int i = 0;
        
        while (s1[i] == s2[i] && s1[i] != '\0')
        {
            i++;
        }

        if (i == stringlen(s1))
            return TRUE;
        return FALSE;
    }
}