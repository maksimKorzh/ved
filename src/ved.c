#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include "ed.h"

/* tab width in spaces */
int TAB_WIDTH = 4;

/* visual editor variables */
int ROWS = 24;
int COLS = 80;
int cury = 0;
int curx = 0;
int tabsx = 0;
int row_offset = 0;
int col_offset = 0;

/* visaul editor's mode (normal/insert) */
char vmode = 'N';

/* visual editor's screen buffer */
struct buffer {
  char *string;
  int len;
};

/* info message */
char info_message[80];

/* display command prompt */
void print_info_message(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(info_message, sizeof(info_message), fmt, ap);
  va_end(ap);
  update_screen();
}

/* print message bar */
void print_message_bar(struct buffer *buf) {
  append_buffer(buf, CLEAR_LINE, 3);
  int msglen = strlen(info_message);
  if (msglen > COLS) msglen = COLS;
  if (msglen) append_buffer(buf, info_message, msglen);
}

/* draw status bar */
void print_status_bar(struct buffer *buf) {
  int fnlen;
  for (fnlen = 0; def_filename[fnlen] != '\0'; fnlen++);
  append_buffer(buf, INVERT_VIDEO, 4);
  char message_left[80]; char message_right[80];
  int len_left = snprintf(message_left,
    sizeof(message_left), "%c %.20s - %d lines %s", vmode, fnlen ? def_filename : "No file",
    last_addr(), modified() ? "[modified]" : "");
  int len_right = snprintf(message_right,
    sizeof(message_right), "Row %d, Col %d ", cury + 1, curx + 1);
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

/* print intro */
int print_intro_line(struct buffer * buf, char *line, int row, int offset, char * arg) {
    if (last_addr() == 0 && row == (ROWS / 3) + offset) {
      char welcome[80];
      int welcomelen = snprintf(welcome, sizeof(welcome), "%s%s", line, arg);
      if (welcomelen > COLS) welcomelen = COLS;
      int padding = (COLS - welcomelen) / 2;
      if (padding) {
        append_buffer(buf, "~", 1);
        padding--;
      }
      while (padding--) append_buffer(buf, " ", 1);
      append_buffer(buf, welcome, welcomelen);
      return 1;
    } else return 0;
}

// render row from text buffer
char *render_row(char *line, int len, int *rlen) {
  int tabs = 0;
  for (int col = 0; col < len; col++) if (line[col] == '\t') tabs++;
  //free(row->render);
  char *render = malloc(len + tabs*(TAB_WIDTH - 1) + 1);
  int index = 0;
  for (int col = 0; col < len; col++) {
    if (line[col] == '\t') {
      render[index++] = ' ';
      while (index % TAB_WIDTH != 0) render[index++] = ' ';
    } else render[index++] = line[col];
  }
  render[index] = '\0';
  *rlen = index;
  return render;
}

/* print display buffer */
void print_buffer(struct buffer *buf) {
  for (int row = 0; row < ROWS; row++) {
    int bufrow = row + row_offset+1;
    if (bufrow >= last_addr()+1) {
           if (print_intro_line(buf, "VED - Visual EDitor", row, 0, "")) {}
      else if (print_intro_line(buf, "version ", row, 2, VERSION)) {}
      else if (print_intro_line(buf, "by Code Monkey King", row, 3, "")) {}
      else if (print_intro_line(buf, "Ved is open source and freely distributable", row, 4, "")) {}
      else if (print_intro_line(buf, "freesoft.for.people@gmail.com", row, 6, "")) {}
      else if (row) append_buffer(buf, "~", 1);
    } else {
      line_t *lp = search_line_node(bufrow);
      char *line = get_sbuf_line(lp);
      int rlen = 0;
      char *rline = render_row(line, lp->len, &rlen);
      int len = rlen - col_offset;
      if (len < 0) len = 0;
      if (len > COLS) len = COLS;
      append_buffer(buf, rline + col_offset, len);
      free(rline);
    }
    append_buffer(buf, CLEAR_LINE, 3);
    append_buffer(buf, "\r\n", 2);
  }
}

/* append string to display buffer */
void append_buffer(struct buffer *buf, const char *string, int len) {
  char *new_string = realloc(buf->string, buf->len + len);
  if (new_string == NULL) return;
  memcpy(&new_string[buf->len], string, len);
  buf->string = new_string;
  buf->len += len;
}

/* render tabs */
int curx_to_tabsx(char *line, int current_x) {
  int render_x = 0;
  for (int col = 0; col < current_x; col++) {
    if (line[col] == '\t') render_x += (TAB_WIDTH - 1) - (render_x % TAB_WIDTH);
    render_x++;
  } return render_x;
}

/* scroll display buffer */
void scroll_buffer() {
  tabsx = 0;
  line_t *lp = search_line_node(cury+1);
  char *line = get_sbuf_line(lp);
  if (cury < last_addr()+1) tabsx = curx_to_tabsx(line, curx);
  if (cury < row_offset) { row_offset = cury; }
  if (cury >= row_offset + ROWS) { row_offset = cury - ROWS + 1; }
  if (tabsx < col_offset) col_offset = tabsx;
  if (tabsx >= col_offset + COLS) col_offset = tabsx - COLS + 1;
}

/* free display buffer */
void clear_buffer(struct buffer *buf) { free(buf->string); }

/* refresh the screen */
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
  if (write(STDOUT_FILENO, buf.string, buf.len)) {/* suppress warning */};
  clear_buffer(&buf);
}

// control cursor
void move_cursor(int key) {
  line_t *row = (cury >= last_addr()) ? NULL : search_line_node(cury+1);
  switch(key) {
    case 'h': if (curx != 0) curx--; break;
    case 'l': if (row && curx < row->len-1) curx++; break;
    case 'k': if (cury != 0) cury--; break;
    case 'j': if (last_addr() && cury != last_addr()-1) cury++; break;
  }
  row = (cury >= last_addr()) ? NULL : search_line_node(cury+1);
  int rowlen = row ? row->len : 0;
  if (curx > rowlen-1) curx = rowlen-1;
}

/* insert new line */
void insert_new_line() {
  line_t *lp = search_line_node(cury+1);
  char *line = get_sbuf_line(lp);  
  if (line != NULL) {
    int leftlen = curx+2;
    int rightlen = lp->len - curx + 2; //print_info_message("\\n: lp->len: %d, is_null: %d", lp->len, line == NULL ? 1 : 0); read_key();
    char *left = malloc(leftlen);
    char *right = malloc(rightlen);
    for (int i = 0; i < leftlen; i++) {
      if (i < curx) left[i] = line[i];
      else if (i == curx) left[i] = '\n';
      else if (i == leftlen-1) left[i] = '\0';
    } const char *cleft = left;
    delete_lines(cury+1, cury+1, false);
    append_lines(&cleft, current_addr(), current_addr() >= cury+1, true);
    for (int i = 0; i < rightlen; i++) {
      if (i < rightlen-2) right[i] = line[i+curx];
      else if (i == rightlen-2) right[i] = '\n';
      else if (i == rightlen-1) right[i] = '\0';
    } const char *cline = rightlen > 2 ? right : "\n";
    append_lines(&cline, current_addr()+1, current_addr()+1 >= cury+1, true);
    cury++; curx = 0; free(left); free(right);
  } else {
    for (int i = 0; i < 2; i++) {
      char uline[] = "\n";
      const char *cline = uline;
      if (i) cury++;
      curx = 0;
      append_lines(&cline, current_addr()+i, current_addr()+i >= cury+1, true);
    }
  }
}

/* inserted char to text buffer row */
void insert_char(int c) {
  line_t *lp = search_line_node(cury+1);
  char *line = get_sbuf_line(lp);  
  //print_info_message("char: lp->len: %d, is_null: %d", lp->len, line == NULL ? 1 : 0); read_key();
  if (line != NULL) {
    int newlen = lp->len+3;
    char *uline = malloc(newlen);
    for (int i = 0; i < newlen; i++) {
      if (i < curx) uline[i] = line[i];
      else if (i == curx) uline[i] = c;
      else if (i == newlen-2) uline[i] = '\n';
      else if (i == newlen-1) uline[i] = '\0';
      else uline[i] = line[i-1];
    } const char *cline = uline;
    delete_lines(cury+1, cury+1, false);
    append_lines(&cline, current_addr(), current_addr() >= cury+1, true);
    free(uline);
    curx++;
  } else {
    char uline[] = " \n"; uline[0] = c;
    const char *cline = uline; curx++;
    append_lines(&cline, current_addr(), current_addr() >= cury+1, true);
  }
}

/* process keypress */
void read_keyboard() {
  int c = read_key();
  if (c == 0x1b) {
    if (vmode == 'I' && curx) curx--;
    vmode = 'N';
  } else if (vmode == 'N') {
    switch(c) {
      case 'h':
      case 'l':
      case 'k':
      case 'j': move_cursor(c); break;
      case ':': mode = 'e'; break;
      case 'i': vmode = 'I'; break;
      case 'a': vmode = 'I'; curx++; break;
      case 'A': vmode = 'I'; curx = search_line_node(cury+1)->len; break;
    }
  } else if (vmode == 'I') {
    if (c == '\n') insert_new_line();
    //case BACKSPACE: if (c == DEL) move_cursor(ARROW_RIGHT); delete_char(); break;
    else if (c != ((c) & 0x1f)) insert_char(c);
  }
}

/* init visual editor */
void init_ved() {
  raw_mode();
  if (get_window_size(&ROWS, &COLS) == -1) die("get_window_size");
  ROWS -= 2;
}

/* visual editor loop */
void ved_loop() {
  while (1) {
    set_current_addr(cury+1);
    update_screen();
    read_keyboard();
    if (mode == 'e') return;
  }
}
