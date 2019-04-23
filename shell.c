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
    char cd[2];
    char comm[128];

    //interrupt(0x21, 0x00, '$', 0x00, 0x00);
    while(1) {
        interrupt(0x21, 0x00, "$ ", 0x00, 0x00); /* print $ */

        interrupt(0x21, 0x01, command, 0x00, 0x00); /* nerima input char dengan nama variabel command */
        
        interrupt(0x21, 0x02, dirs, 257, 0x00); /* read sector dirs */
        
        // split command ke cd
        cd[0] = command[0]; // cd berisi "cd"
        cd[1] = command[1]; 
        if (stringcmp(command, "cd..") == TRUE || stringcmp(command, "cd ..") == TRUE) {
            if (curdir != USED) {
                // pindah ke parent dir
                curdir = dirs[curdir*MAX_SECTORS];
            }
        }
        else if (stringcmp(cd, "cd")) { //implementasi cd
            // split command setelah " " ke comm
            int i = 3;
            while (command[i] != '\0') { //comm berisi argumen command
                comm[i-3] = command[i];
                i++;
            }
            // pindah ke child dir dengan nama yang sesuai inputan 
            i = 0;
            int ketemu = FALSE;
            while (i*16 <= SECTOR_SIZE || !(ketemu)) {
                if (dirs[i*MAX_SECTORS] == curdir) {
                    int j = i*MAX_SECTORS+1; // idx ngebaca dirs 
                    char dir[MAX_SECTORS];
                    int k = 0;
                    while (command[j] != '\0') { // masukin dirs ke dir biar bisa di compare
                        dir[k] = dirs[j];
                        k++;
                        j++;
                    }
                    if (stringlen(dir) == stringlen(comm)) {
                        if (stringcmp(dir,comm)) {
                            curdir = i; /* pindah directory */
                            ketemu = TRUE; /* stop while loop */
                        }
                    }
                }
            }
            if (!ketemu) {
                interrupt(0x21, 0x00, "Directory not found", 0x00, 0x00);
            }        
        }
        else { 
            if (stringcmp(cd, "./")) {
                int i = 2;
                while (command[i] != ' ' && command[i] != '\0') { //comm berisi program yang ingin di eksekusi
                    comm[i] = command[i];
                    i++;
                }
                if (command[i] != '\0') {
                    i++; 
                    argc = 0;
                    while (command[i] != '\0') {
                        int k = 0; // indeks argv[argc]
                        while (command[i] != ' ') {
                            argv[argc][k] = command[i];
                            k++;
                            i++;
                        }
                        argc++;
                        i++;
                    }
                    interrupt(0x21, 0x20, USED, argc, argv); // put args
                }
                interrupt(0x21, 0x06, comm, 0x21000, &result); // execute program
            }
            else {
                int i = 0;
                while (command[i] != ' ' && command[i] != '\0') { //comm berisi program yang ingin di eksekusi
                    comm[i] = command[i];
                    i++;
                }
                if (command[i] != '\0') {
                    i++; 
                    argc = 0;
                    while (command[i] != '\0') {
                        int k = 0; // indeks argv[argc]
                        while (command[i] != ' ') {
                            argv[argc][k] = command[i];
                            k++;
                            i++;
                        }
                        argc++;
                        i++;
                    }
                    interrupt(0x21, 0x20, USED, argc, argv); // put args
                }
                interrupt(0x21, 0x06, comm, 0x21000, &result); // execute program
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