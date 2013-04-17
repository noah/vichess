#ifndef VICHESS_H
#define VICHESS_H

#define _GNU_SOURCE

#define TITLE "♟ vichess ♙"

// message queue constants
#define MAX_LINE_SIZE   8192 // corresponds to rlimit
#define PRIORITY        0

// general purpose macros
#define LEN(x)  (sizeof(x) / sizeof(x[0]))
#define UNUSED(x) (void)(x)

#include <assert.h>     // assert()
#include <ctype.h>      // tolower()
#include <errno.h>      // errno
#include <fcntl.h>      // O_* bits
#include <limits.h>     // INT_MAX
#include <locale.h>     // utf-8
#include <math.h>       // floor()
#include <mqueue.h>     // mq_*
#include <ncurses.h>    // curses - includes stdio.h, unctrl.h, stdarg.h, stddef.h
#include <netdb.h>      // struct addrinfo
#include <pthread.h>    // pthread_create
#include <signal.h>     // signals
#include <stdbool.h>    // bool, true, false
#include <stdlib.h>     // exit()
#include <string.h>     // memset(), strtok(), strdup()
#include <sys/socket.h> // sockets
#include <sys/stat.h>   // S_* bits
#include <unistd.h>     // close()

// struct to contain file descriptors (or ids, as in the case of
// message queues) and other config data
typedef struct CONFIG
{
  int sk;
  mqd_t ib_mq;
  mqd_t ob_mq;
  WINDOW *w1;
  WINDOW *w2;
  WINDOW *w3;
} CONFIG;


/* utils.c */

bool begins_with(char *, char *);
bool contains(char *, char *);
bool equals(char *, char *);
bool even(int);
bool odd(int);
long *get_socket_fd(const char *, const char *);
mqd_t *get_mq_fd(const char* , int);
ssize_t read_line(int , void *, size_t); 
unsigned int centered(char *);
void debug(const char *, ...);
void error(const char *);
void *get_in_addr(struct sockaddr *);
void ms_to_hh_mm_ss_ms(int, char *);
void set_realpath(char *, char *);
void swap(char**, char **);

/* client.c, term.c */

void t_socket_line_reader(void *);
void t_socket_line_writer(void *);
void t_curses_term_reader(void *);
void t_curses_term_writer(void *);
//
void cb_term_resize(int);
void send_message(mqd_t, char *, ...);

/* callbacks.c */

// A chess board with n^2 cells where each cell has cell width w and
// cell height has size (wn+1)(hn+1).
#define N_ROWS          8
#define N_COLS          8
#define CENTER          COLS/2
#define SQUARE_WIDTH    3

void cb_read_command(WINDOW *, void *);
void cb_write_board(WINDOW *, void *);
void cb_write_gameinfo(WINDOW *, void *);
void cb_write_response(WINDOW *, void *);

/* Output line numbers, relative to the curses Y coordinate.
 *
 *
 * this prints something like:
 *__________________________________________________________________...
 * 0            vichess
 * 1            observing 1 +0 
 * 2            lightning #42 rated
 * 3            usera (rating) 00:00:01.133 <-
 * 4  .-----------------------------------.
 *    |  chessboard ...                    |
 *    |                                   |
 *    ...                                 |
 *    '___________________________________' <- BOARD_START_LINE + N_ROWS -1
 *              userb (rating) 00:00:10.282
 */
enum __INTERFACE_LINE_NUMBERS
{
  TITLE_LINE        = 0,
  GAME_INFO_LINE    = 1,
  OPP_INFO_LINE     = 2,
  BOARD_START_LINE  = 3,
  BOARD_END_LINE    = BOARD_START_LINE + N_ROWS - 1,
  MY_INFO_LINE      = BOARD_END_LINE + 1

};

/* fics.c */

// A good resource: http://www.freechess.org/Help/AllFiles.html

#define SERVER      "freechess.org"
#define PORT        "23"
#define FICS_PROMPT "fics%"
#define FICS_QUIT   "quit"

// update markers
#define STYLE12_MARKER  "<12>"
#define GAMEINFO_MARKER "<g1>"

enum __PIECE_COLORS
{
  WHITE=-1,
  BLACK=1
};

enum __BOARD_ORIENTATIONS
{
  PLAYING_AS_WHITE=0,
  PLAYING_AS_BLACK=1
};

// game play modes
enum __MODES
{
  IDLE                      = 42,
  // the following values correspond to my_status in Style12 struct
  ISOLATED                  = -3,
  OBSERVING_EXAMINATION     = -2,
  EXAMINING                 =  2, 
  PLAYING_OPPONENTS_MOVE    = -1, 
  PLAYING_MY_MOVE           =  1,
  OBSERVING                 =  0
};

// color handles, see vichess.c for assignments
enum __CURSES_COLORS
{
  CLI_INPUT,
  TERMINAL,
  LIGHT_ON_DARK,
  GREENISH,
  QUIET_DARK,
  DARK_SQUARE,
  LIGHT_SQUARE,
  GRAY_ON_BLACK,
  BLUISH,
  RED
};

// unicode piece and box drawing character definitions
//
#define EMPTY_SQUARE    " "
//
#define BLACK_R         "♜"
#define BLACK_N         "♞"
#define BLACK_B         "♝"
#define BLACK_Q         "♛"
#define BLACK_K         "♚"
#define BLACK_P         "♟"
//
#define WHITE_R         "♖"
#define WHITE_N         "♘"
#define WHITE_B         "♗"
#define WHITE_Q         "♕"
#define WHITE_K         "♔"
#define WHITE_P         "♙"
//
#define LIGHTNING       "☇"
#define BLITZ           "⚒"
#define STANDARD        "☎"
#define UNTIMED         "∞"
// 
#define FINGER          "☚"

// from http://www.freechess.org/Help/HelpFiles/register.html :
//
//    "[Handles] must be at least 3 characters long and cannot exceed
//    17 characters."
#define NICK_MAX        18 // 17 + '\0'
#define STR_TIME_LEN    13 // strlen("hh:mm:ss.000\0") => 13


typedef struct UPDATE
{

  // the unparsed text of this update messgae
  char * text;

  /* Gameinfo message-specific fields */
  //
  //
  //
  unsigned int game_number;

  //
  char * type;
  char * type_sym;

  //
  bool rated;

  //
  char * white_rating;
  char * black_rating;
  char * my_rating;
  char * opp_rating;

  //
  bool white_timeseal;
  bool black_timeseal;

  //
  //
  //
  bool private;
  //
  bool white_registered;
  bool black_registered;
  //
  unsigned int white_initial_time;
  unsigned int black_initial_time;
  //
  unsigned int white_initial_inc;
  unsigned int black_initial_inc;
  //
  unsigned int partner_game_number;


  /* Style 12 message-specific fields */
  // 
  //
  char * my_nick;
  char * opp_nick;
  // 
  // BLACK or WHITE
  unsigned int my_color;
  // 
  bool my_turn;
  //
  bool i_can_castle_long;
  bool i_can_castle_short;
  bool opp_can_castle_long;
  bool opp_can_castle_short;
  //
  int my_strength;
  int opp_strength;
  //
  // remaining time in milliseconds
  unsigned int my_ms;
  unsigned int opp_ms;

  //
  //
  int my_status;
  char * my_status_str;
  // 
  // game time limit
  //    e.g., 3 2 (blitz) or 60 0 (standard)
  unsigned int match_minutes;
  unsigned int match_increment;
  //
  // the chess board
  char *board[N_COLS][N_ROWS];
  char *old_board[N_COLS][N_ROWS];

} UPDATE;

char *char_to_piece(char );
void parse_gameinfo_string(const char *, UPDATE *);
void parse_s12_string(const char *, UPDATE *);
void print_g1(UPDATE *);
void print_s12(UPDATE *);

#endif
