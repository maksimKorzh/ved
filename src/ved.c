#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include "ed.h"

/* visual editor variables */
int ROWS = 24;
int COLS = 80;
int cury = 0;
int curx = 0;
int tabsx = 0;
int lastx = 0;
int row_offset = 0;
int col_offset = 0;

/* visual editor's 'screen' */
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
  //if (msglen && time(NULL) - info_time < 5)
  if (msglen) append_buffer(buf, info_message, msglen);
}

/* draw status bar */
void print_status_bar(struct buffer *buf) {
  append_buffer(buf, INVERT_VIDEO, 4);
  char message_left[80]; char message_right[80];
  int len_left = 0;//snprintf(message_left, sizeof(message_left), "%.20s - %d lines %s", filename ? filename : "[No file]", total_lines, modified ? "[modified]" : "");
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

/* print display buffer */
void print_buffer(struct buffer *buf) {
  for (int row = 0; row < ROWS; row++) {
    int bufrow = row + row_offset-1;
    if (bufrow >= last_addr()+1) {
           if (print_intro_line(buf, "VED - Visual EDitor", row, 0, "")) {}
      else if (print_intro_line(buf, "version ", row, 2, VERSION)) {}
      else if (print_intro_line(buf, "by Code Monkey King", row, 3, "")) {}
      else if (print_intro_line(buf, "Ved is open source and freely distributable", row, 4, "")) {}
      else if (print_intro_line(buf, "freesoft.for.people@gmail.com", row, 6, "")) {}
      else if (row > 2) {
        append_buffer(buf, "~", 1);
      }
    } else {
      line_t *lp = search_line_node(bufrow);
      char *line = get_sbuf_line(lp);
      int len = lp->len - col_offset;
      if (len < 0) len = 0;
      if (len > COLS) len = COLS;
      append_buffer(buf, line + col_offset, len);
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

/* scroll display buffer */
void scroll_buffer() {
  tabsx = 0;
  //if (cury < total_lines) tabsx = curx_to_tabsx(&text[cury], curx);
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
  write(STDOUT_FILENO, buf.string, buf.len);
  clear_buffer(&buf);
}

/* process keypress */
void read_keyboard(int loose) {
  int c = read_key();
  switch(c) {
    //case (('q') & 0x1f): clear_screen(); exit(0); break;
    /*case ':': {
        int status = ed_loop(loose);
        if (status) {
            clear_screen();
            exit(status);
        }
    }*/
    //case '\r': insert_new_line(); break;
    //case BACKSPACE: if (c == DEL) move_cursor(ARROW_RIGHT); delete_char(); break;
    //default: insert_char(c); break;
  }
}

/* init visual editor */
void init_ved() {
  raw_mode();
  //if (get_window_size(&ROWS, &COLS) == -1) die("get_window_size");
  //ROWS -= 2; print_info_message("    QUIT: Ctrl-q | NEW: Ctrl-n | OPEN: Ctrl-O | SAVE: Ctrl-s | SHELL: Ctrl-e");
}

/* visual editor loop */
int ved_loop(int loose) {
  //return 0;
  /*while (1) {
    update_screen();
    read_keyboard(loose);
  }*/
}
