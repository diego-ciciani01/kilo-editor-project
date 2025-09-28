#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

/**** Data ****/
struct termios orig_termios;

void die(const  char *s){
    perror(s);
    exit(1);
}

void disableRawMode(void){
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1){
        die("tcsetattr");
    }
}
void enableRawMode(void){
    struct termios raw;
    if(tcgetattr(STDIN_FILENO, &orig_termios)){
        die("tcgetattr");
    }
    atexit(disableRawMode);

    raw = orig_termios;
    raw.c_iflag &= ~(ICRNL | IXON  | ISTRIP | IXON | BRKINT);
    raw.c_oflag &= ~(OPOST);
    raw. c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN |  ISIG);

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1){
        die("tcsetattr");
    }

    raw.c_cc[VMIN] = 0; /*paramiter to wait a set prefixed time*/
    raw.c_cc[VTIME] = 1;
}
/**** Init ****/
int main(void){
    enableRawMode();
    while(1){
        char c = '\0';
        /* con 'errno != EAGAIN' escludiamo il caso in c'è un time out, vogliamo semplicemnte leggere il prossimo
         *  carattere1
         * */
        if(read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN){
            die("read");
        }
        if(iscntrl(c)){
            printf("%d\r\n", c);
        }else{
            printf("%d ('%c')\r\n", c,c);
          }
        if (c == 'q')
            break;
    }
	return 0;
}
