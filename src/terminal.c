#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "ed.h"

/* original terminal settings */
struct termios coocked_mode;

/* error handling */
void die(const char *message) {
  perror(message);
  printf("\r");
  exit(1);
}

/* clear terminal screen */
void clear_screen() {
  if (write(STDOUT_FILENO, CLEAR_SCREEN, 4)) {/* suppress warning */};
  if (write(STDOUT_FILENO, RESET_CURSOR, 3)) {/* suppress warning */};
}

/* enable terminal raw mode */
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

/* restore terminal back to default mode */
void restore_terminal() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &coocked_mode); }

/* get terminal window size */
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

/* get cursor ROW, COL position */
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

