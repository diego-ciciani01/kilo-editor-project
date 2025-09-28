#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>

// 0x1f = 0001111, CTRL + K strips bit 5 and 6 from ascii encoding of k
#define CTRL_KEY(k) ( (k) & 0x1f)

/**** Data ****/
struct editorConfig{
    int stringrows;
    int stringcols;
    struct termios orig_termios;
};

struct editorConfig E;

void die(const  char *s){
    write(STDIN_FILENO, "\x1b[2J", 4);
    write(STDIN_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}
/****** terminal ******/
void disableRawMode(void){
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1){
        die("tcsetattr");
    }
}
void enableRawMode(void){
    struct termios raw;
    if(tcgetattr(STDIN_FILENO, &E.orig_termios)){
        die("tcgetattr");
    }
    atexit(disableRawMode);

    raw = E.orig_termios;
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

char editorReadKey(void){
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

int getWindowSize(int *rows, int *cols){
    struct winsize ws;
    // ioct interact with dispositive, in this case the size of the windows
    // TIOCGWINSZ is talling give me the size
    if(ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1 ||  ws.ws_col == 0){
        return -1;
    }else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}
/***** output ******/
void editorDrawRows(void){
    int y;
    // 24, screen dimension
    for(y=0; y<24;y++){
        write(STDIN_FILENO, "~\r\n", 3);
    }

}


/*
 * \x1b -> 1b = esc
 * [
 * 2 -> clean the entire screen
 * J -> clean
 * */
void editorRefreshScreen(void){
    //so here we are going to write a sequance (escape sequance) of byte on the standard output, file descriptor
    // this escape sequance are used for format the text editor, we are using J command, to clean the screan
    write(STDIN_FILENO, "\x1b[2J", 4);
    write(STDIN_FILENO, "\x1b[H", 3);

    editorDrawRows();
    write(STDIN_FILENO, "\x1b[H", 3);
}


/****** input *****/
void editorProcessKeyPress(void){
    char c = editorReadKey();
    switch(c){
        case CTRL_KEY('q'): {
            write(STDIN_FILENO, "\x1b[2J", 4);
            write(STDIN_FILENO, "\x1b[H", 3);

            exit(0);
            break;
    }
    }

}

/**** Init ****/
void initEditor(void){
    if (getWindowSize(&E.stringrows, &E.stringcols) == -1){
        die("getWindowSize()");
    }
}

int main(void){
    enableRawMode();
    initEditor();
    while(1){
        editorRefreshScreen();
        editorProcessKeyPress();
    }
	return 0;
}
