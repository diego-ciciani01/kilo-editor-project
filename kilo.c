#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

/***** Define *****/
// 0x1f = 0001111, CTRL + K strips bit 5 and 6 from ascii encoding of k
#define CTRL_KEY(k) ( (k) & 0x1f)
#define KILO_VERSION  "0.0.1"
/**** Data ****/
struct editorConfig{
    int stringrows; // dimension windows paramiter
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

int getCursorPosition(int *rows, int *cols){
    char buf[32];
    unsigned int i = 0;
    if(write(STDIN_FILENO, "\x1b[6n", 4) != 4){
        return -1;
    }
    while(i < sizeof(buf) - 1){
        if(read(STDIN_FILENO, &buf[i], 1) != 1 ) break;

        if(buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';
    if(buf[0] != '\x1b' || buf[1] != '[') return -1;
    if(sscanf(buf +2, "%d;%d", rows, cols) != 2) return -1;
    return 0;
}

int getWindowSize(int *rows, int *cols){
    struct winsize ws;
    // ioct interact with dispositive, in this case the size of the windows
    // TIOCGWINSZ is talling give me the size
    // this  is a second tentative for get size
    if(ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1 ||  ws.ws_col == 0){
        // this is alternative way, to get windows size if 'H' method fail
        // ask for cursor position
        if(write(STDIN_FILENO, "\x1b[999C\x1b[999B", 12) != 12){
            return -1;
        }
        return getCursorPosition(rows, cols);
    }else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/**** append buffer****/
//this struct is used to handle the string
struct abuf {
    char *b;
    int len;
};

#define ABUF_INIT{NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len){
    char *new = realloc(ab->b, ab->len+len);
    if(new==NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len = ab->len + len;

}
void abFree(struct abuf *ab){
    free(ab->b);
}



/***** output ******/
void editorDrawRows(struct abuf *ab){
    int y;
    // 24, screen dimension
    for(y=0; y<E.stringrows;y++){
         if(y == E.stringrows / 3){
                char welcome[80];
                int welcomelen = snprintf(welcome,
                                    sizeof(welcome),
                                    "kilo editor --version %s", KILO_VERSION);
                if(welcomelen > E.stringcols) welcomelen = E.stringcols;
                int padding=(E.stringcols - welcomelen)/2;
                if(padding) {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while(padding --) abAppend(ab, " ", 1);
                abAppend(ab, welcome, welcomelen);
         }else{
            abAppend(ab, "~", 1);
         }
        abAppend(ab, "\x1b[k", 3); //delate the row before
        if(y<E.stringrows -1){
            abAppend(ab, "\r\n",2);
        }
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
    // i do all the append of string in the buffer, and then i do write
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);
    abAppend(&ab, "\x1b[H", 3);
    abAppend(&ab, "\x1b[25h", 6);  //hide and show the cursor


    write(STDIN_FILENO, ab.b, ab.len);
    abFree(&ab);
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
    editorRefreshScreen();
    enableRawMode();
    initEditor();
    printf("%d, %d\n",E.stringrows, E.stringcols);

    while(1){
        editorRefreshScreen();
        editorProcessKeyPress();
    }

	return 0;
}
