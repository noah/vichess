#include "vichess.h"

/*
 * ncurses is not thread-safe.
 *
 * In practice, this means that multiple threads must synchronize access
 * to WINDOW and SCREEN objects.  While it is possible to do this
 * "manually" with a mutexes, it is easier/cleaner to let the
 * use_{window, screen} functions in ncurses.h manage the synch.  See
 * curs_threads(3X), which provides that:
 *
 *      The use_window and use_screen functions provide coarse
 *      granularity mutexes for their respective WINDOW and SCREEN
 *      parameters, and call a user-supplied function, passing it a data
 *      parameter, and re‐ turning the value from the user-supplied
 *      function to the application.
 *
 * Basically what this means is that rather than synchronizing threads
 * using a pthread mutex lock, a thread can call the use_window function
 * as follows when it wants to synchronize an update to a window:
 *
 *      use_window( mywindow, (NCURSES_WINDOW_CB) some_cb_fun, data);
 *
 * Where some_cb_fun is a function like:
 *
 *      void some_cb_fun(WINDOW *w, void *data)
 *      {
 *          waddstr(w, (char *) data);
 *      }
 * */

void cb_read_command(WINDOW *w, void *data)
{
  // (blockingly) read a line of input from the CLI window
  char *d = (char *) data;
  char input[MAX_LINE_SIZE-1];
  wgetnstr(w, input, MAX_LINE_SIZE-1);
  snprintf(d, MAX_LINE_SIZE, "%s\n", input); // append newline
  // reset the input window
  wclrtoeol(w); 
  wmove(w, 0, 0);
  wrefresh(w);
}

void cb_write_response(WINDOW *w, void *data)
{
  // match strings with these prefixes
  static char *highlight_prefixes[] = {
      "{Game ",
      "Game ",
      "    **ANNOUNCEMENT**",
      "Removing game ",
      "Notification: ",
      "Creating: ",
      "No ratings adjustment done.",
      "Your seek matches one",
      "You are now observing",
      "(told ",
      "% "
  };
  // match strings containing these substrings
  static char *highlight_contains[] = {
      " tells you: ",
      " kibitzes: ",
      "(U)(",
      "(TD)(",
      "(C)(",
  };

  char *line = (char *) data;

  // highlight matches
  for (int i=0; i < LEN(highlight_prefixes); i++)
    if ( begins_with(line, highlight_prefixes[i]) )
      wattron(w, COLOR_PAIR( GREENISH ));
  for (int i=0; i < LEN(highlight_contains); i++)
    if ( contains(line, highlight_contains[i]) )
      wattron(w, COLOR_PAIR( GRAY_ON_BLACK ));

  waddstr(w, line); 
  wstandend(w);
  wrefresh(w);
}


/*
 * = Callbacks for writing data to the windows
 *
 * Window udpates are a little tortured, because of the way FICS sends
 * updates.
 *
 * There are 2 basic update messages that both contain a subset of
 * important match and player information:
 *
 *  - Style12Update message (contains board state and player nicknames)
 *  - GameinfoUpdate message (contains player ratings)
 *
 * We want to display all of the pertinent info, but these messages
 * arrive at different intervals, in potentially arbitrary order, and
 * in disproportionate number.
 *
 * So, it doesn't make sense to do any message queueing client-side.
 * Not to mention that lightning/blitz players will want the information
 * in front of their eyes ASAP...
 *
 * So, the appraoch here is to allow callbacks to perform window updates
 * whenever the data arrives, being careful to never allow two threads
 * to write to the same region of the window. This works, but is not
 * very robust, as the write locations are basically hard-coded.
 *
 * */


void cb_write_board(WINDOW *w, void *data)
{
  UPDATE *u = (UPDATE*) data;

  // ok, here we go -- update the gui
  
  //    reusable buffers
  char hh_mm_ss_ms[STR_TIME_LEN], update_line[COLS]; 
  memset(hh_mm_ss_ms, 0, STR_TIME_LEN);

  // update game info line
  memset(update_line, 0, COLS); wmove(w, GAME_INFO_LINE, 0); wclrtoeol(w);
  sprintf(update_line, "%s %s game #%d, %d +%d %s", 
                        u->my_status_str,
                        u->type, 
                        u->game_number, 
                        u->match_minutes,
                        u->match_increment,
                        u->rated ? "rated" : "unrated");
  mvwprintw(w, GAME_INFO_LINE, centered(update_line), update_line);

  // update opponent info line
  //
  memset(update_line, 0, COLS); wmove(w, OPP_INFO_LINE, 0); wclrtoeol(w);
  ms_to_hh_mm_ss_ms(u->opp_ms, hh_mm_ss_ms);
  sprintf(update_line, "%s (%s) %s %s", u->opp_nick, u->opp_rating, hh_mm_ss_ms, ( ! u->my_turn ? FINGER : "   "));
  mvwprintw(w, OPP_INFO_LINE, centered(update_line), update_line);

  // update board
  //    pad board for display
  char *board[N_ROWS][N_COLS*SQUARE_WIDTH];
  memset(board, 0, sizeof(*board));
  for (int row = 0; row < N_ROWS; row++)
    for (int col = 0; col < N_COLS*SQUARE_WIDTH; col++)
      board[row][col] = ((col - 1) % SQUARE_WIDTH == 0) 
        ? u->board[row][(col/SQUARE_WIDTH)]
        : EMPTY_SQUARE ;

  //    determine the window width and thereby, the proper indentation
  int w_y, w_x, h_indent, v_indent; getmaxyx(w, w_y, w_x);
  h_indent = (w_x - N_ROWS * SQUARE_WIDTH) / 2;
  v_indent = BOARD_START_LINE;
  UNUSED( w_y) ;

  //    print the board
  for (int row = 0; row < N_ROWS; row++)
  {
    bool black_square = even(row) ? true : false;
    wmove(w, row+v_indent, h_indent);
    for (int col = 0; col < N_COLS*SQUARE_WIDTH; col++)
    {
      // flip square color at square borders
      if ((col % SQUARE_WIDTH) == 0)
      {
        black_square = ! black_square;
      }
      if (black_square)   wattron(w, COLOR_PAIR( DARK_SQUARE    ) );
      else                wattron(w, COLOR_PAIR( LIGHT_SQUARE   ) );

      // highlight last move FIXME
      /*
      if (...)
      {
        wattron(w, COLOR_PAIR( RED ));
        waddstr(w, "X");
      }
      else
        waddstr(w, board[row][col]);
      */
      waddstr(w, board[row][col]);
      wstandend(w);
    }
  }

  // update my info line
  //
  memset(update_line, 0, COLS); wmove(w, MY_INFO_LINE, 0); wclrtoeol(w);
  ms_to_hh_mm_ss_ms(u->my_ms, hh_mm_ss_ms);
  sprintf(update_line, "%s (%s) %s %s", u->my_nick, u->my_rating, hh_mm_ss_ms, (u->my_turn ? FINGER : "   "));
  mvwprintw(w, MY_INFO_LINE, centered(update_line), update_line);

  // clean up formatting and refresh the gui
  wstandend(w);
  wrefresh(w);
}
