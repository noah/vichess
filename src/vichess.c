#include "vichess.h"

const char *QUEUES[] = { "/ob", "/ib" };
void unlink_queues() { for (int i = 0; i < LEN(QUEUES); i++) { mq_unlink( QUEUES[i] ); } }

/*
  // curses emits SIGWINCH upon window resize -- register callback
  //signal(SIGWINCH, handle_term_resize); //FIXME
 * Signal handler (still a callback, but caller is signal(), not thread)
void handle_term_resize(int _)
{ 
  endwin();
  refresh();
  wrefresh(w1);
  wrefresh(w2);
  wrefresh(w3);
  clear();
  wclear(w1);
  wclear(w2);
  wrefresh(w1);
  wrefresh(w2);
  wrefresh(w3);
  refresh();
}
* */

WINDOW *w1, *w2, *w3;

void initialize_curses()
{
  // start curses
  initscr();
  if (  ! has_colors() || (start_color() != OK) || COLORS != 256 ) error("colors");

  // Define three horizontally-stacked windows:
  // 
  //    1 - board window
  //    2 - CLI output window
  //    3 - CLI input window
  //
  int half_height = LINES / 2;
  //   newwin( height,          width,  begin_height,   begin_width    );
  w1 = newwin( half_height,     COLS,      0,              0              );
  w2 = newwin( half_height - 1, COLS,      half_height,    0              );
  w3 = newwin( 1,               COLS,      LINES - 1,      0              );
  if (w1 == NULL || w2 == NULL || w3 == NULL) { waddstr(stdscr, "newwin"); endwin(); }

  echo(); scrollok(w2, TRUE);

  // initialize color tuples
  //   see http://www.calmar.ws/vim/256-xterm-24bit-rgb-color-chart.html
  //                            FG,     BG
  init_pair(    LIGHT_SQUARE,   0,      229     );
  init_pair(    DARK_SQUARE,    0,      209     );
  init_pair(    BLUISH,         111,    233     );
  init_pair(    TERMINAL,       252,    232     );
  init_pair(    GREENISH,       107,    233     );
  init_pair(    CLI_INPUT,      253,    232     ); 
  init_pair(    GRAY_ON_BLACK,  245,    233     ); 
  init_pair(    RED,            1,      52      ); 

  wbkgd( w1, COLOR_PAIR( TERMINAL  ) );
  wbkgd( w2, COLOR_PAIR( TERMINAL  ) );
  wbkgd( w3, COLOR_PAIR( CLI_INPUT ) );

  wattron(w1, COLOR_PAIR( BLUISH) );
  mvwaddstr(w1, 0, centered(TITLE), TITLE);
  wstandend(w1);

  wrefresh(stdscr); wrefresh(w1); wrefresh(w2); wrefresh(w3);
  touchwin(stdscr); touchwin(w1); touchwin(w2); touchwin(w3);
}

int main(int argc, char *argv[])
{
  if (setlocale( LC_ALL, "en_US.utf8" ) == NULL) error("setlocale");

  initialize_curses();

  unlink_queues(); // delete any stale queues

  // central data structure contains pointers to various components --
  // sockets, message queues, and curses windows.
  //
  mqd_t *ob_mq  = get_mq_fd(QUEUES[0], O_CREAT | O_RDWR);
  mqd_t *ib_mq  = get_mq_fd(QUEUES[1], O_CREAT | O_RDWR);
  long *sock_fd = get_socket_fd( SERVER, PORT );
  CONFIG config = 
  { 
    .ob_mq      = *ob_mq, 
    .ib_mq      = *ib_mq, 
    .sk         = *sock_fd, 
    .w1         = w1, 
    .w2         = w2, 
    .w3         = w3,
  };

  // define an array of worker threads
  //
  void (*workers[]) = 
  {
    t_socket_line_writer,   // reads from message queue, writes to socket
    t_socket_line_reader,   // reads from socket, writes to message queue
    t_curses_term_writer,   // reads from message queue, writes to term
    t_curses_term_reader,   // reads from term, writes to message queue
  };
  
  // launch threads and wait for them to complete their work
  //
  pthread_t T[ LEN( workers ) ];
  for (int t = 0; t < LEN(T); t++) { pthread_create(&T[t],  NULL, workers[t], &config); }
  for (int i = 0; i < LEN(T); i++) { pthread_join(T[i],     NULL); }

  // Clean up file handles
  close(config.sk);
  mq_close(config.ob_mq); 
  mq_close(config.ib_mq);
  free(ob_mq);
  free(ib_mq);
  free(sock_fd);

  // clean up curses
  delwin(w1);
  delwin(w2);
  delwin(w3);
  unlink_queues();
  endwin();

  return 0;
}
