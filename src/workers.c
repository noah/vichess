#include "vichess.h"

static bool running = true;

void t_socket_line_writer(void *config) // write messages to socket
{
  CONFIG *c = (CONFIG *) config;
  while ( running )
  {
    // TODO window resizing
    // int y1, x1, y2, x2, y3, x3; getmaxyx(c->w1, y1, x1); getmaxyx(c->w2, y2, x2); getmaxyx(c->w3, y3, x3);
    char recv_buf[MAX_LINE_SIZE]; memset(recv_buf, 0, MAX_LINE_SIZE);
    if ( mq_receive(c->ob_mq, recv_buf, MAX_LINE_SIZE, 0) == -1 )   error("mq_recv");
    if ( send(c->sk, recv_buf, strlen(recv_buf), 0) == -1)          error("send");
  }
}

void t_socket_line_reader(void *config) // read messages from socket
{
  CONFIG *c         = (CONFIG*) config;
  long message_id   = 0;
  bool configured   = false;

  while ( running )
  {
    char line_buf[MAX_LINE_SIZE];
    memset(line_buf, 0, MAX_LINE_SIZE);

    // read a message (1 line) from the socket and increment message count
    if (read_line(c->sk, line_buf, MAX_LINE_SIZE) < 1) 
    { 
      running = false; // server socket closed
      break;
    } 
    message_id++;

    // Telnet server may have latency, so sleep()-and-send() doesn't
    // work, and switching on message_id is no less brittle than
    // grepping for magic strings...
    // debug("message: %d %s", message_id, line_buf); 
    switch (message_id)
    {
      case 26: send_message( c->ob_mq, "guest\n"  );  break;
      case 27: send_message( c->ob_mq, "\n\n"     );  break;
      case 28: 
         // we're logged in; it should be OK to send commands to the
         // server
         if ( ! configured )
         {
          int w_y, w_x; getmaxyx(c->w2, w_y, w_x);
          send_message( c->ob_mq, "set height %d\n", w_y    );  // server-side paging height
          send_message( c->ob_mq, "set width %d\n", w_x     );  // server-side paging width
          send_message( c->ob_mq, "iset nowrap 1\n"         );  // don't wrap lines (breaks linewise hilighting)
          send_message( c->ob_mq, "iset gameinfo 1\n"       );  // request game information
          send_message( c->ob_mq, "iset ms 1\n"             );  // request timing in milliseconds
          send_message( c->ob_mq, "-channel 53\n"           );  // remove guest chat from channel list
          send_message( c->ob_mq, "set prompt %\n"          );  // a simpler prompt
          send_message( c->ob_mq, "set style 12\n"          );  // computer-readable output format
          send_message( c->ob_mq, "set seek 0\n"            );  // no seek advertisements TODO seek graph (?)
          send_message( c->ob_mq, "set bell off\n"          );  // bell off
          send_message( c->ob_mq, "set provshow 1\n"        );  // annotate provisional and estimated ratings
          send_message( c->ob_mq, "set interface %s\n", TITLE );
          configured = true;
         } 
         break;
      default:
         // user is now logged-in.  Handle any messages
         if ( begins_with(line_buf, "\a" ) )    continue;   // skip bells and empty prompts
         if ( begins_with(line_buf, "% \a" ) )  continue;
         if ( begins_with(line_buf, "% \n" ) )  continue;
         if ( equals(line_buf, FICS_PROMPT) )   continue;
         break;
    } 
    if ((mq_send(c->ib_mq, line_buf, MAX_LINE_SIZE, PRIORITY)) == -1) perror("mq_send");
  }
}

void t_curses_term_writer(void *config) // write messages to terminal
{
  CONFIG *c = (CONFIG*) config;
  UPDATE *u = malloc(sizeof *u); if ( u == NULL ) error("update malloc");

  // track changes matrix out as false
  bool changed[N_ROWS][N_COLS];// = { [0 ... N_ROWS-1][0 ... N_COLS-1] = false };

  int s12, g1, _ = 0;
  while ( running )
  {
    // 
    char recv_buf[MAX_LINE_SIZE];
    memset(recv_buf, 0, MAX_LINE_SIZE);

    int MODE = IDLE;

    if ( mq_receive(c->ib_mq, recv_buf, MAX_LINE_SIZE, 0) == -1 ) error("mq_receive");

    // peek into the message and handle appropriately
    //
    if ( begins_with(recv_buf, GAMEINFO_MARKER) )
    {
      parse_gameinfo_string( recv_buf, u );
      g1++;
    }
    else if ( begins_with(recv_buf, STYLE12_MARKER) )
    { 
      // make a copy of the existing ("old") board
      memset(u->old_board, 0, sizeof u->old_board);
      memcpy(u->old_board, u->board, sizeof u->board);

      // parse the new board
      parse_s12_string( recv_buf, u );

      if (s12 > 0) // this is not the first message
      {
        // assume new board has not changed
        memset(changed, false, sizeof changed);
        // compare new board to old board
        for (int i=0; i<N_ROWS; i++)
          for (int j=0; j<N_ROWS; j++)
            changed[i][j] = (bool) (u->old_board[i][j] != u->board[i][j]);
      }

      MODE = u->my_status;
      if ( ! ( u->white_rating == NULL) ) // have gameinfo
        use_window(c->w1, (NCURSES_WINDOW_CB) cb_write_board, u);

      s12++;
    }
    else
    { 
      // normal line, no parsing necessary.  write to w2
      use_window(c->w2, (NCURSES_WINDOW_CB) cb_write_response, recv_buf);
      _++;
    }

    // handle cursor placement in a mode-dependent way
    switch ( MODE )
    {
      case PLAYING_MY_MOVE:
      case PLAYING_OPPONENTS_MOVE:
        // TODO move cursor to last remaining piece, or king pawn if
        // first move, etc
        break;
      case OBSERVING: 
      case IDLE:
        // move cursor to the input line
        // TODO command history (?)
        wmove(c->w3, LINES, COLS);
        wrefresh(c->w3);
        break;
      default: break;
    }
  }
  // clear dynamic memory
  free(u->my_nick);
  free(u->opp_nick);
  free(u->text);
  free(u->type);
  free(u->white_rating);
  free(u->black_rating);
  free(u);
  u = NULL;
}

void t_curses_term_reader(void *config) // read messages from terminal
{
  CONFIG *c = (CONFIG*) config;
  while ( running )
  {
    char command_buf[MAX_LINE_SIZE];
    memset(command_buf, 0, MAX_LINE_SIZE);
    use_window(c->w3, (NCURSES_WINDOW_CB) cb_read_command, command_buf);
    if ( strlen(command_buf) < 1 )          continue;
    // user command to the server AND echo to the screen
    send_message(c->ob_mq, command_buf);
    send_message(c->ib_mq, command_buf);
    if ( begins_with(command_buf, FICS_QUIT) ) running = false;
  }
}
