#include "kernel.c"
#define TRUE 1
#define FALSE 0

int main() {
    char curdir = 0xFF; /* inisialisasi */
    char command[128];
    char buffer[512];
    char dirs[512];
    char argc;
    char argv[4][16];
    
    
    while(1) {
        interrupt(0x21, 0x00, '$', 0x00, 0x00); /* print $ */
        
        interrupt(0x21, 0x01, command, 0x00, 0x00); /* nerima input char dengan nama variabel command */
        
        interrupt(0x21, 0x02, dirs, 257, 0x00); /* read sector dirs */
        
        if (command[0] ==  'c' && command[1] == 'd') {/* implementasi cd */
            if (command[2] == '.' && command[3] == '.') {
                if (curdir != 0xFF) {
                    /* pindah ke parent dir */
                    curdir = dirs[curdir*16];
                }
            }
            else if (command[2] == ' ') {
                if (command[3] == '.' && command[4] == '.') {
                    if (curdir != 0xFF) {
                        /* pindah ke parent dir */
                        curdir = dirs[curdir*16];
                    }
                }
                else {
                    /* pindah ke child dir dengan nama yang sesuai inputan */
                    int i = 0;
                    int j,k;
                    int ketemu = FALSE;
                    while (i*16 <= 512 || !(ketemu)) {
                        if (dirs[i*16] == curdir) {
                            j = i*16+1; /* idx ngebaca dirs */
                            k = 3; /* idx ngebaca command */
                            while (j <= i*16+15 || (dirs[j] != '\0' && command[k] != '\0')) {
                                if (dirs[j] == command[k]) {
                                    j++;
                                    k++;
                                }
                                if ((j == i*16+15) && (command[k+1] == '\0')) { /* kasus kalo dirs udh diujung dan command udh diujung juga */
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
        }
        else { /* menjalankan program */
            interrupt(0x21, 0x21, &curdir, 0, 0); /* getCurdir */
            interrupt(0x21, 0x22, &argc, 0, 0); /* getArgc */
            int i;
            for (i = 0; i < argc; ++i) {
                interrupt(0x21, 0x23, i, argv[i], 0); /* getArgv */
            }
            interrupt(0x21, 0x20, curdir, argc, argv); /* setArgs */
            interrupt(0x21, 0x20, curdir, argc, argv); /* putArgs */
            
            
        }
    }
}
