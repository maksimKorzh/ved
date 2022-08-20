/* Global declarations for the ed editor.  */
/* GNU ed - The GNU line editor.
   Copyright (C) 1993, 1994 Andrew Moore, Talke Studio
   Copyright (C) 2006-2022 Antonio Diaz Diaz.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define VERSION "0.0.1"

#ifndef __cplusplus
enum Bool { false = 0, true = 1 };
typedef enum Bool bool;
#endif

enum Pflags			/* print suffixes */
  {
  pf_l = 0x01,			/* list after command */
  pf_n = 0x02,			/* enumerate after command */
  pf_p = 0x04			/* print after command */
  };


typedef struct line		/* Line node */
  {
  struct line * q_forw;
  struct line * q_back;
  long pos;			/* position of text in scratch buffer */
  int len;			/* length of line ('\n' is not stored) */
  }
line_t;


typedef struct
  {
  enum { UADD = 0, UDEL = 1, UMOV = 2, VMOV = 3 } type;
  line_t * head;			/* head of list */
  line_t * tail;			/* tail of list */
  }
undo_t;

#ifndef max
#define max( a, b ) ( (( a ) > ( b )) ? ( a ) : ( b ) )
#endif
#ifndef min
#define min( a, b ) ( (( a ) < ( b )) ? ( a ) : ( b ) )
#endif

static const char * const mem_msg = "Memory exhausted";
static const char * const no_prev_subst = "No previous substitution";

// ANSI escape sequences
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

/* Special keys */
#define BACKSPACE 127

/* visual screen buffer */
struct buffer;

/* tab width in spaces */
int TAB_WIDTH;

/* visual editor variables */
int ROWS;
int COLS;
int cury;
int curx;
int oldy;
int oldx;
int tabsx;
int row_offset;
int col_offset;

/* command/visual modes*/
char mode;

/* current file name */
const char * def_filename;

/* defined in buffer.c */
bool append_lines( const char ** const ibufpp, const int addr,
                   bool insert, const bool isglobal );
bool close_sbuf( void );
bool copy_lines( const int first_addr, const int second_addr, const int addr );
int current_addr( void );
int dec_addr( int addr );
bool delete_lines( const int from, const int to, const bool isglobal );
int get_line_node_addr( const line_t * const lp );
char * get_sbuf_line( const line_t * const lp );
int inc_addr( int addr );
int inc_current_addr( void );
bool init_buffers( void );
bool isbinary( void );
bool join_lines( const int from, const int to, const bool isglobal );
int last_addr( void );
bool modified( void );
bool move_lines( const int first_addr, const int second_addr, const int addr,
                 const bool isglobal );
bool open_sbuf( void );
int path_max( const char * filename );
bool put_lines( const int addr );
const char * put_sbuf_line( const char * const buf, const int size );
line_t * search_line_node( const int addr );
void set_binary( void );
void set_current_addr( const int addr );
void set_modified( const bool m );
bool yank_lines( const int from, const int to );
void clear_undo_stack( void );
undo_t * push_undo_atom( const int type, const int from, const int to );
void reset_undo_state( void );
bool undo( const bool isglobal );

/* defined in global.c */
void clear_active_list( void );
const line_t * next_active_node( void );
bool set_active_node( const line_t * const lp );
void unset_active_nodes( const line_t * bp, const line_t * const ep );

/* defined in io.c */
bool get_extended_line( const char ** const ibufpp, int * const lenp,
                        const bool strip_escaped_newlines );
const char * get_stdin_line( int * const sizep );
int linenum( void );
bool print_lines( int from, const int to, const int pflags );
int read_file( const char * const filename, const int addr );
int write_file( const char * const filename, const char * const mode,
                const int from, const int to );
void reset_unterminated_line( void );
void unmark_unterminated_line( const line_t * const lp );
int read_key();
char *command_prompt(char *command, int *lenp);

/* defined in main.c */
bool extended_regexp( void );
bool is_regular_file( const int fd );
bool may_access_filename( const char * const name );
bool restricted( void );
bool scripted( void );
void show_strerror( const char * const filename, const int errcode );
bool strip_cr( void );
bool traditional( void );

/* defined in ed.c */
void invalid_address( void );
int main_loop( const bool initial_error, const bool loose );
bool set_def_filename( const char * const s );
void set_error_msg( const char * const msg );
bool set_prompt( const char * const s );
void set_verbose( void );
void unmark_line_node( const line_t * const lp );

/* defined in regex.c */
bool build_active_list( const char ** const ibufpp, const int first_addr,
                        const int second_addr, const bool match );
const char * get_pattern_for_s( const char ** const ibufpp );
bool extract_replacement( const char ** const ibufpp, const bool isglobal );
int next_matching_node_addr( const char ** const ibufpp );
bool search_and_replace( const int first_addr, const int second_addr,
                         const int snum, const bool isglobal );
bool set_subst_regex( const char * const pat, const bool ignore_case );
bool replace_subst_re_by_search_re( void );
bool subst_regex( void );

/* defined in signal.c */
void disable_interrupts( void );
void enable_interrupts( void );
bool resize_buffer( char ** const buf, int * const size, const unsigned min_size );
void set_signals( void );
void set_window_lines( const int lines );
const char * strip_escapes( const char * p );
int window_columns( void );
int window_lines( void );

/* defined in terminal.c */
void die(const char *message);
void clear_screen();
void raw_mode();
void restore_terminal();
int get_window_size(int *rows, int *cols);
int get_cursor(int *rows, int * cols);

/* defined in ved.c */
void print_info_message(const char *fmt, ...);
void print_message_bar(struct buffer *buf);
void print_status_bar(struct buffer *buf);
char *render_row(char *line, int len, int *rlen);
void print_buffer(struct buffer *buf);
void append_buffer(struct buffer *buf, const char *string, int len);
int curx_to_tabsx(char *line, int current_x);
void scroll_buffer();
void clear_buffer(struct buffer *buf);
void update_screen();
void move_cursor(int key);
void insert_new_line();
void delete_char();
void insert_char(int c);
void replace_char(int c);
void read_keyboard();
void init_ved();
void ved_loop();

