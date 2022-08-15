/****************************************\
 ========================================
        Minimalist terminal based
          text editor for linux
 ========================================
\****************************************/

// libraries
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>

// editor definition
#define CONTROL(k) ((k) & 0x1f)
#define CLEAR_LINE "\x1b[K"
#define GET_CURSOR "\x1b[6n"
#define SET_CURSOR "\x1b[%d;%dH"
#define CURSOR_MAX "\x1b[999C\x1b[999B"
#define HIDE_CURSOR "\x1b[?25l"
#define SHOW_CURSOR "\x1b[?25h"
#define INVERT_VIDEO "\x1b[7m"
#define RESET_CURSOR "\x1b[H"
#define CLEAR_SCREEN "\x1b[2J"
#define RESTORE_VIDEO "\x1b[m"

/****************************************\
 ========================================
                 SETTINGS
 ========================================
\****************************************/

// tab width in spaces
int TAB_WIDTH = 4;

// special keys
enum { BACKSPACE = 127, ARROW_LEFT = 1000, ARROW_RIGHT, ARROW_UP, ARROW_DOWN, PAGE_UP, PAGE_DOWN, HOME, END, DEL };
int keygroup_1[] = { HOME, -1, DEL, END, PAGE_UP, PAGE_DOWN, HOME, END };
int keygroup_2[] = { ARROW_UP, ARROW_DOWN, ARROW_RIGHT, ARROW_LEFT, -1, END, -1, HOME };
int keygroup_3[] = { END, -1, HOME};

// editor variables
int ROWS = 80;
int COLS = 24;
int cury = 0;
int curx = 0;
int tabsx = 0;
int lastx = 0;
int userx = 0;
int total_lines = 0;
int row_offset = 0;
int col_offset = 0;
int modified = 0;

// editor's 'screen'
struct buffer {
  char *string;
  int len;
};

// line of text representation
typedef struct text_buffer {
  char *string;
  char *render;
  int len;
  int rlen;
} text_buffer;

// editor's lines of text
text_buffer *text = NULL;

// current filename
char *filename = NULL;

// info message
char info_message[80];
time_t info_time = 0;

// original terminal settings
struct termios coocked_mode;


/****************************************\
 ========================================
                FUNCTIONS
 ========================================
\****************************************/

// terminal
void die(const char *message);
void clear_screen();
void raw_mode();
void restore_terminal();
 int get_window_size(int *rows, int *cols);
 int get_cursor(int *rows, int * cols);
void move_cursor(int key);
 int read_key();
void read_keyboard();

// editor
void append_buffer(struct buffer *buf, const char *string, int len);
void clear_buffer(struct buffer *buf);
void print_buffer(struct buffer *buf);
void scroll_buffer();
void insert_new_line();
void update_row(text_buffer *row);
void insert_row(int row, char *string, size_t len);
void delete_row(int row);
void free_row(text_buffer *row);
void insert_row_char(text_buffer *row, int col, int c);
void delete_row_char(text_buffer *row, int col);
 int curx_to_tabsx(text_buffer *row, int current_x);
void insert_char(int c);
void delete_char();
void append_string(text_buffer *row, char *string, size_t len);
char *buffer_to_string(int *buffer_len);
void print_status_bar(struct buffer *buf);
void print_info_message(const char *fmt, ...);
void print_message_bar(struct buffer *buf);
void update_screen();
void init_editor();

// file I/O
void open_file(char *file_name);
void save_file();
void new_file();

// system integration
char *command_prompt(char *command);
void execute();

/****************************************\
 ========================================
                 TERMINAL
 ========================================
\****************************************/

// error handling
void die(const char *message) {
  perror(message);
  printf("\r");
  exit(1);
}

// clear terminal screen
void clear_screen() {
  write(STDOUT_FILENO, CLEAR_SCREEN, 4);
  write(STDOUT_FILENO, RESET_CURSOR, 3);
}

// enable terminal raw mode
void raw_mode() {
  if (tcgetattr(STDIN_FILENO, &coocked_mode) == -1) die("tcgetattr");
  atexit(restore_terminal);
  struct termios raw_mode = coocked_mode;
  raw_mode.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  raw_mode.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw_mode.c_oflag &= ~(OPOST);
  raw_mode.c_cflag |= (CS8);
  raw_mode.c_cc[VMIN] = 0;
  raw_mode.c_cc[VTIME] = 1;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_mode) == -1) die("tcsetattr");
}

// restore terminal back to default mode
void restore_terminal() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &coocked_mode); }

// get terminal window size
int get_window_size(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, CURSOR_MAX, 12) != 12) return -1;
    return get_cursor(rows, cols);
  } else {
    *rows = ws.ws_row;
    *cols = ws.ws_col;
    return 0;
  }
}

// get cursor ROW, COL position
int get_cursor(int *rows, int * cols) {
  char buf[32];
  unsigned int i = 0;
  if (write(STDOUT_FILENO, GET_CURSOR, 4) != 4) return -1;
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  } buf[i] = '\0';
  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
  return 0;
}

// control cursor
void move_cursor(int key) {
  lastx = userx;
  text_buffer *row = (cury >= total_lines) ? NULL : &text[cury];
  switch(key) {
    case ARROW_LEFT:
      if (curx != 0) { curx--; userx--; }
      else if (cury > 0) {
        cury--;
        curx = text[cury].len;
      } break;
    case ARROW_RIGHT:
      if (row && curx < row->len) { curx++; userx++; }
      else if (row && curx == row->len && cury != total_lines - 1) {
        cury++;
        curx = 0;
      } break;
    case ARROW_UP:
      if (cury != 0) {
        cury--;
        curx = lastx;
      } else curx = 0;
      break;
    case ARROW_DOWN:
      if (total_lines && cury != total_lines - 1) {
        cury++;
        curx = lastx;
      } else if (cury == total_lines - 1) curx = text[total_lines].rlen;
      break;
  }
  row = (cury >= total_lines) ? NULL : &text[cury];
  int rowlen = row ? row->len : 0;
  if (curx > rowlen) curx = rowlen;
}

// read a single key from STDIN
int read_key() {
  int bytes;
  char c;
  while ((bytes = read(STDIN_FILENO, &c, 1)) != 1)
  if (bytes == -1 && errno != EAGAIN) die("read");
  if (c == '\x1b') {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        if (seq[2] == '~') return keygroup_1[seq[1] - '1'];
      } else return keygroup_2[seq[1] - 'A'];
    } else if (seq[0] == 'O') {
      return keygroup_3[seq[1] - 'F'];
    } return '\x1b';
  } else return c;
}

// process keypress
void read_keyboard() {
  int c = read_key();
  switch(c) {
    case '\r': insert_new_line(); break;
    case CONTROL('n'): new_file(); break;
    case CONTROL('o'): {
      char *name = command_prompt("Open file > %s");
      if (name != NULL) open_file(name); break;
    }
    case CONTROL('e'): execute(); break;
    case CONTROL('q'): clear_screen(); free(text); exit(0); break;
    case CONTROL('s'): save_file(); break;
    case HOME: curx = 0; break;
    case END: if(cury < total_lines) curx = text[cury].len; break;
    case DEL:
    case BACKSPACE: if (c == DEL) move_cursor(ARROW_RIGHT); delete_char(); break;
    case PAGE_UP:
    case PAGE_DOWN: {
      if (c == PAGE_UP) cury = row_offset;
      else if (c == PAGE_DOWN) {
        cury = row_offset + ROWS - 1;
        if (cury > total_lines) cury = total_lines;
      } int times = ROWS;
      while (times--) move_cursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
    } break;
    case ARROW_LEFT:
    case ARROW_RIGHT:
    case ARROW_UP:
    case ARROW_DOWN: move_cursor(c); break;
    case CONTROL('l'):
    case '\x1b': break;
    default: insert_char(c); break;
  }
}


/****************************************\
 ========================================
                  EDITOR
 ========================================
\****************************************/

// append string to display buffer
void append_buffer(struct buffer *buf, const char *string, int len) {
  char *new_string = realloc(buf->string, buf->len + len);
  if (new_string == NULL) return;
  memcpy(&new_string[buf->len], string, len);
  buf->string = new_string;
  buf->len += len;
}

// free display buffer
void clear_buffer(struct buffer *buf) { free(buf->string); }

// print display buffer
void print_buffer(struct buffer *buf) {
  for (int row = 0; row < ROWS; row++) {
    int bufrow = row + row_offset;
    if (bufrow < total_lines) {
      int len = text[bufrow].rlen - col_offset;
      if (len < 0) len = 0;
      if (len > COLS) len = COLS;
      append_buffer(buf, &text[bufrow].render[col_offset], len);
    }
    append_buffer(buf, CLEAR_LINE, 3);
    append_buffer(buf, "\r\n", 2);
  }
}

// scroll display buffer
void scroll_buffer() {
  tabsx = 0;
  if (cury < total_lines) tabsx = curx_to_tabsx(&text[cury], curx);
  if (cury < row_offset) { row_offset = cury; }
  if (cury >= row_offset + ROWS) { row_offset = cury - ROWS + 1; }
  if (tabsx < col_offset) col_offset = tabsx;
  if (tabsx >= col_offset + COLS) col_offset = tabsx - COLS + 1;
}

// insert new line to text buffer
void insert_new_line() {
  if (curx == 0) insert_row(cury, "", 0);
  else {
    text_buffer *row = &text[cury];
    insert_row(cury + 1, &row->string[curx], row->len - curx);
    row = &text[cury];
    row->len = curx;
    row->string[row->len] = '\0';
    update_row(row);
  }
  cury++;
  curx = 0; userx = 0;
}

// render row from text buffer
void update_row(text_buffer *row) {
  int tabs = 0;
  for (int col = 0; col < row->len; col++) if (row->string[col] == '\t') tabs++;
  free(row->render);
  row->render = malloc(row->len + tabs*(TAB_WIDTH - 1) + 1);
  int index = 0;
  for (int col = 0; col < row->len; col++) {
    if (row->string[col] == '\t') {
      row->render[index++] = ' ';
      while (index % TAB_WIDTH != 0) row->render[index++] = ' ';
    } else row->render[index++] = row->string[col];
  }
  row->render[index] = '\0';
  row->rlen = index;
}

// insert new row to text buffer
void insert_row(int row, char *string, size_t len) {
  if (row < 0 || row > total_lines) return;
  text = realloc(text, sizeof(text_buffer) * (total_lines + 1));
  memmove(&text[row + 1], &text[row], sizeof(text_buffer) * (total_lines - row));
  text[row].string = malloc(len + 1);
  text[row].len = len;
  memcpy(text[row].string, string, len);
  text[row].string[len] = '\0';
  text[row].render = NULL;
  text[row].rlen = 0;
  update_row(&text[row]);
  total_lines++;
  modified++;
}

// delete row from text buffer
void delete_row(int row) {
  if (row < 0 || row >= total_lines) return;
  free_row(&text[row]);
  memmove(&text[row], &text[row + 1], sizeof(text_buffer) * (total_lines - row - 1));
  total_lines--;
  modified++;
}

// free memory for text buffer row
void free_row(text_buffer *row) {
  free(row->string);
  free(row->render);
}

// inserted char to text buffer row
void insert_row_char(text_buffer *row, int col, int c) {
  if (col < 0 || col > row->len) col = row->len;
  row->string = realloc(row->string, row->len + 2);
  memmove(&row->string[col + 1], &row->string[col], row->len - col + 1);
  row->len++;
  row->string[col] = c;
  update_row(row);
  modified++;
}

// deleted char from text buffer row
void delete_row_char(text_buffer *row, int col) {
  if (col < 0 || col >= row->len) return;
  memmove(&row->string[col], &row->string[col + 1], row->len - col);
  row->len--;
  update_row(row);
  modified++;
}

// render tabs
int curx_to_tabsx(text_buffer *row, int current_x) {
  int render_x = 0;
  for (int col = 0; col < current_x; col++) {
    if (row->string[col] == '\t') render_x += (TAB_WIDTH - 1) - (render_x % TAB_WIDTH);
    render_x++;
  } return render_x;
}

// call insert char to text buffer row
void insert_char(int c) {
  if (cury == total_lines) insert_row(total_lines, "", 0);
  insert_row_char(&text[cury], curx, c);
  curx++;
  userx++;
}

// call delete char from text buffer row
void delete_char() {
  //if (cury == total_lines) return;
  if (curx == 0 && cury == 0) return;
  text_buffer *row = &text[cury];
  if (curx > 0) {
    delete_row_char(row, curx - 1);
    curx--;
    userx--;
  } else {
    curx = text[cury - 1].len; userx = curx;
    append_string(&text[cury - 1], row->string, row->len);
    delete_row(cury);
    cury--;
  }
}

// append a string to text buffer row
void append_string(text_buffer *row, char *string, size_t len) {
  row->string = realloc(row->string, row->len + len + 1);
  memcpy(&row->string[row->len], string, len);
  row->len += len;
  row->string[row->len] = '\0';
  update_row(row);
  modified++;
}

// convert text buffer to string
char *buffer_to_string(int *buffer_len) {
  int total_len = 0;
  for (int row = 0; row < total_lines; row++)
    total_len += text[row].len + 1;
  *buffer_len = total_len;
  char *buffer_string = malloc(total_len);
  char *p = buffer_string;
  for (int row = 0; row < total_lines; row++) {
    memcpy(p, text[row].string, text[row].len);
    p += text[row].len;
    *p = '\n';
    p++;
  } return buffer_string;
}

// draw status bar
void print_status_bar(struct buffer *buf) {
  append_buffer(buf, INVERT_VIDEO, 4);
  char message_left[80]; char message_right[80];
  int len_left = snprintf(message_left, sizeof(message_left), "%.20s - %d lines %s", filename ? filename : "[No file]", total_lines, modified ? "[modified]" : "");
  int len_right = snprintf(message_right, sizeof(message_right), "Row %d, Col %d", cury + 1, curx + 1);
  if (len_left > COLS) len_left = COLS;
  append_buffer(buf, message_left, len_left);
  while (len_left < COLS) {
    if (COLS - len_left == len_right) {
      append_buffer(buf, message_right, len_right);
      break;
    } else {
		  append_buffer(buf, " ", 1);
		  len_left++;
    }
  }
  append_buffer(buf, RESTORE_VIDEO, 3);
  append_buffer(buf, "\r\n", 2);
}

// print a message under the status bar
void print_info_message(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(info_message, sizeof(info_message), fmt, ap);
  va_end(ap);
  info_time = time(NULL);
}

// print message bar
void print_message_bar(struct buffer *buf) {
  append_buffer(buf, CLEAR_LINE, 3);
  int msglen = strlen(info_message);
  if (msglen > COLS) msglen = COLS;
  if (msglen && time(NULL) - info_time < 5)
    append_buffer(buf, info_message, msglen);
}

// refresh the screen
void update_screen() {
  scroll_buffer();
  struct buffer buf = {NULL, 0};
  append_buffer(&buf, HIDE_CURSOR, 6);
  append_buffer(&buf, RESET_CURSOR, 3);
  print_buffer(&buf);
  print_status_bar(&buf);
  print_message_bar(&buf);
  char curpos[32];
  snprintf(curpos, sizeof(curpos), SET_CURSOR, (cury - row_offset) + 1, (tabsx - col_offset) + 1);
  append_buffer(&buf, curpos, strlen(curpos));
  append_buffer(&buf, SHOW_CURSOR, 6);
  write(STDOUT_FILENO, buf.string, buf.len);
  clear_buffer(&buf);
}

// init text editor
void init_editor() {
  raw_mode();
  if (get_window_size(&ROWS, &COLS) == -1) die("get_window_size");
  ROWS -= 2; print_info_message("    QUIT: Ctrl-q | NEW: Ctrl-n | OPEN: Ctrl-O | SAVE: Ctrl-s | SHELL: Ctrl-e");
  insert_new_line(); delete_char();
}

/****************************************\
 ========================================
                 FILE I/O
 ========================================
\****************************************/

// read file from disk
void open_file(char *file_name) {
  new_file();
  free(filename);
  filename = strdup(file_name);
  FILE *fp = fopen(file_name, "r");
  if (!fp) return;
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  int old_total_lines = total_lines;
  while ((linelen = getline(&line, &linecap, fp)) != -1) {  
    while (linelen > 0 && (line[linelen-1] == '\n' || line[linelen-1] == '\r')) linelen--;
    insert_row(total_lines, line, linelen);
  } if (old_total_lines) { move_cursor(ARROW_DOWN); delete_char(); }
  free(line);
  fclose(fp);
  modified = 0;
}

// write file to disk
void save_file() {
  if (filename == NULL) filename = command_prompt("Save file > %s");
  if (filename == NULL) { print_info_message("Save aborted"); return; }
  int len;
  char *buffer = buffer_to_string(&len);
  int fd = open(filename, O_RDWR | O_CREAT, 0644);
  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buffer, len) == len) {
        close(fd);
        free(buffer);
        modified = 0;
        print_info_message("%d bytes written to disk", len);
        return;
      }
    } close(fd);
  } free(buffer);
  print_info_message("Failed to save file! I/O error: %s", strerror(errno));
}

// start new file
void new_file() {
  if (filename != NULL) { free(filename); filename = NULL; }
  while (cury < total_lines - 1) move_cursor(ARROW_DOWN);
  move_cursor(END);
  while (cury != 0) delete_char();
  while (curx != 0) delete_char();
  modified = 0;
}


/****************************************\
 ========================================
            SYSTEM INTEGRATION
 ========================================
\****************************************/

// enter user command
char *command_prompt(char *command) {
  size_t bufsize = 128;
  char *buf = malloc(bufsize);
  size_t buflen = 0;
  buf[0] = '\0';
  while(1) {
    print_info_message(command, buf);
    update_screen();
    int c = read_key();
    if (c == BACKSPACE) { if (buflen != 0) buf[--buflen] = '\0'; }
    else if (c == '\x1b') {
      print_info_message("");
      free(buf);
      return NULL;
    } else if (c == '\r') {
      if (buflen != 0) {
        print_info_message("");
        return buf;
      }
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }
  }
}

// execute system command
void execute() {
  char *command = command_prompt("$ %s");
  clear_screen();
  restore_terminal();
  if (command != NULL) system(command);
  printf("Press enter to continue...\n");
  read_key(); raw_mode();
}


/****************************************\
 ========================================
                   MAIN
 ========================================
\****************************************/

int main(int argc, char *argv[]) {
  init_editor(); if (argc >= 2) open_file(argv[1]);
  while (1) { update_screen(); read_keyboard(); }
}
