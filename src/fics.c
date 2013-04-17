#include "vichess.h"

// Convert an ASCII piece to a unicode piece.
char *char_to_piece(char piece)
{
  switch ( piece )
  {
    case 'r': return BLACK_R;
    case 'n': return BLACK_N;
    case 'b': return BLACK_B;
    case 'q': return BLACK_Q;
    case 'k': return BLACK_K;
    case 'p': return BLACK_P;
    //
    case 'R': return WHITE_R;
    case 'N': return WHITE_N;
    case 'B': return WHITE_B;
    case 'Q': return WHITE_Q;
    case 'K': return WHITE_K;
    case 'P': return WHITE_P;
    //
    case '-': return EMPTY_SQUARE;
    //
    default: error("unknown piece"); break;
  }
  error("unicode piece mapping failed");
  return 0;
}

char *type_to_sym(char *type)
{
  switch ( type[0] )
  {
    case 'l': return LIGHTNING;
    case 'b': return BLITZ;
    case 's': return STANDARD;
    case 'u': return UNTIMED;
    default: break;
  }
  error("type to symbol mapping failed");
  return 0;
}

char *status_to_str(int status)
{
  switch ( status )
  {
    case IDLE:                      return "idle";
    case ISOLATED:                  return "isolated";
    case EXAMINING:                 return "examining";
    case OBSERVING:                 return "observing";
    case OBSERVING_EXAMINATION:     return "observing examination";
    case PLAYING_OPPONENTS_MOVE:    
    case PLAYING_MY_MOVE:           return "playing";
  }
  error("status to string mapping failed");
  return 0;
}


/// Style 12
//
//
// n.b. strdup()s must be free()d !
void parse_s12_string(const char line[MAX_LINE_SIZE], UPDATE *u)
{
  /* begin parsing */

  char          *cp         = strdupa(line);
  const char    *delim      = " ";
  // 
  //  Parse Style12 messaging specification, 
  //  http://www.freechess.org/Help/HelpFiles/style12.html
  //  
  //  Example message:
  //  
  //  "<12> rnbqkb-r pppppppp -----n-- -------- ----P--- -------- PPPPKPPP RNBQ-BNR B -1 0 0 1 1 0 7 Newton Einstein 1 2 12 39 39 119 122 2 K/e1-e2 (0:06) Ke2 0"
  //   
  //  This string always begins on a new line, and there are always exactly 31 non-
  //  empty fields separated by blanks. The fields are:
  //  
  //  - the string "<12>" to identify this line.
  char * marker = strtok(cp, delim);
  // 
  //  - eight fields representing the board position.  The first one is White's
  //    etc, regardless of who's move it is.
  char *row, *board[N_ROWS][N_COLS];
  for (int i = 0; i<N_ROWS; i++)
  { 
    memset(row, N_COLS, 0);
    row = strtok(NULL, delim);
    for (int j=0; j < strlen(row); j++)
      board[i][j] = char_to_piece(row[j]);
  }
  //
  //  - color whose turn it is to move ("B" or "W")
  char turn                                     = strtok(NULL, delim)[0];
  //
  //  - -1 if the previous move was NOT a double pawn push, otherwise the chess 
  //   board file  (numbered 0--7 for a--h) in which the double push was made
  int double_push                               = atoi(strtok(NULL, delim));
  //
  //  - can White still castle short? (0=no, 1=yes) 
  //  - can White still castle long?
  //  - can Black still castle short?
  //  - can Black still castle long?
  bool white_can_castle_short                   = atoi(strtok(NULL, delim));
  bool white_can_castle_long                    = atoi(strtok(NULL, delim));
  bool black_can_castle_short                   = atoi(strtok(NULL, delim));
  bool black_can_castle_long                    = atoi(strtok(NULL, delim));
  //
  //  - the number of moves made since the last irreversible move.  (0 if last move
  //    was irreversible.  If the value is >= 100, the game can be declared a draw
  //    due to the 50 move rule.)
  unsigned int moves_since_irreversible_move    = atoi(strtok(NULL, delim));
  //
  //  - The game number
  unsigned int game_number                      = atoi(strtok(NULL, delim));
  //
  //  - White's handle 
  //  - Black's handle
  char * whites_nick                            = strtok(NULL, delim);
  char * blacks_nick                            = strtok(NULL, delim);
  //
  //  - my relation to this game:
  //      -3 isolated position, such as for "ref 3" or the "sposition" command
  //      -2 I am observing game being examined
  //       2 I am the examiner of this game
  //      -1 I am playing, it is my opponent's move
  //       1 I am playing and it is my move
  //       0 I am observing a game being played 
  int my_status                                 = atoi(strtok(NULL, delim));
  //  
  //  - initial time (in seconds) of the match
  unsigned int match_minutes                    = atoi(strtok(NULL, delim));
  //
  //  - increment In seconds) of the match
  unsigned int match_increment                  = atoi(strtok(NULL, delim));
  //  
  //  - White material strength
  //  - Black material strength
  int white_strength                            = atoi(strtok(NULL, delim));
  int black_strength                            = atoi(strtok(NULL, delim));
  //  
  //  - White's remaining time
  //  - Black's remaining time
  int white_seconds                             = atoi(strtok(NULL, delim));
  int black_seconds                             = atoi(strtok(NULL, delim));
  //
  //  - the number of the move about to be made (standard chess
  //  numbering -- White's and Black's first moves are both 1, etc.)
  unsigned int move_number                      = atoi(strtok(NULL, delim));
  //  
  //  - verbose coordinate notation for the previous move ("none" if
  //  there were none) [note this used to be broken for examined games]
  char * verbose_notation                       = strtok(NULL, delim);
  //    
  //    - time taken to make previous move "(min:sec)".
  char * elapsed_previous_move                  = strtok(NULL, delim);
  //    
  //    - pretty notation for the previous move ("none" if there is none)
  char * pretty_notation                        = strtok(NULL, delim);
  //    
  //    - flip field for board orientation: 1 = Black at bottom, 0 = White at bottom.
  unsigned int board_orientation                = atoi(strtok(NULL, delim));

  /* end parsing */

  /* begin assignment */

  //Style12Update *update = malloc(sizeof(*update)); if (update == NULL) { error("parse s12 'lloc"); }

  // suppress compiler warnings for unused tokens
  UNUSED( pretty_notation   );
  UNUSED( verbose_notation  );
  UNUSED( game_number       );
  UNUSED( moves_since_irreversible_move );
  UNUSED( marker );
  UNUSED( double_push );
  UNUSED( elapsed_previous_move );
  UNUSED( move_number );

  // 
  // Convert absolute player positioning (white/black) to relative
  // positioning (player/opponent).
  //
  //    When the above talks about players, it phrases things in terms
  //    of "white this" and "black that".  It also give us a "board
  //    orientation" flag and prints the board upside-down in case we
  //    are playing as black.  The following reverses that logic, and
  //    makes everything relative to I/me/my/Player (who we're logged-in
  //    as) and opp/Opponent (the guy across the board).  It makes no
  //    difference to my program whether we're playing as white or
  //    black.  By doing it this way I avoid putting a bunch of
  //    branching logic in later when it comes time to display the board
  //    and information about the players.  i.e., no (if white_on_top then
  //    ...)
  //
  //    (However, because board_orientation is not present in the
  //    Gameinfo message, branching logic cannot be avoided where that
  //    struct is concerned.)
  //    
  // FIXME -- is this still necessary if we "set flip"?
  //
  switch ( board_orientation )
  {
    case PLAYING_AS_BLACK:

      // in-place "rotate" the upside-down board 180 deg
      //    two loops:  reverse rows; reverse columns = O(2*(N*n/2)) = O(n^2))
      for (int i=0; i<N_ROWS; i++) for (int j=0; j<N_ROWS/2; j++) swap(&board[i][j], &board[i][N_ROWS-j-1]);
      for (int j=0; j<N_ROWS; j++) for (int i=0; i<N_ROWS/2; i++) swap(&board[i][j], &board[N_ROWS-i-1][j]);

      //
      u->my_nick                = strdup(blacks_nick);
      u->opp_nick               = strdup(whites_nick);
      u->my_color               = BLACK;
      u->my_turn                = (turn == 'B');
      u->i_can_castle_long      = black_can_castle_long;
      u->i_can_castle_short     = black_can_castle_short;
      u->opp_can_castle_long    = white_can_castle_long;
      u->opp_can_castle_short   = white_can_castle_short;
      u->my_strength            = black_strength;
      u->opp_strength           = white_strength;
      u->my_ms                  = black_seconds;
      u->opp_ms                 = white_seconds;
      u->my_rating              = strdup(u->black_rating);
      u->opp_rating             = strdup(u->white_rating);
      break;

    case PLAYING_AS_WHITE:
      
      //
      u->my_nick                = strdup(whites_nick);
      u->opp_nick               = strdup(blacks_nick);
      u->my_color               = WHITE;
      u->my_turn                = (turn == 'W');
      u->i_can_castle_long      = white_can_castle_long;
      u->i_can_castle_short     = white_can_castle_short;
      u->opp_can_castle_long    = black_can_castle_long;
      u->opp_can_castle_short   = black_can_castle_short;
      u->my_strength            = white_strength;
      u->opp_strength           = black_strength;
      u->my_ms                  = white_seconds;
      u->opp_ms                 = black_seconds;
      u->my_rating              = strdup(u->white_rating);
      u->opp_rating             = strdup(u->black_rating);
      break;
      
    default:
      break;
  }

  // Set fields that do not depend on board orientation
  //
  u->text              = strdup(line);
  u->my_status         = my_status;
  u->my_status_str     = strdup(status_to_str( my_status ));
  u->match_minutes     = match_minutes;
  u->match_increment   = match_increment;
  memcpy(u->board, board, sizeof board);

  /* end assignment */

  //return update;
}


/// Gameinfo

void parse_gameinfo_string(const char line[MAX_LINE_SIZE], UPDATE *u)
{
  /* begin parsing */
  char *cp                  = strdupa(line);
  //  
  //  Gameinfo messaging specification, 
  //  http://www.freechess.org/Help/HelpFiles/iv_gameinfo.html
  //
  //  Note:  some of this is redundant with the Style12 message (see
  //  below); however, rating and timeseal are not in S12, and that is
  //  interesting.
  //
  //  Example message:
  //
  //  "<g1> 77 p=0 t=lightning r=1 u=0,0 it=60,0 i=60,0 pt=0 rt=1880,1789 ts=1,1 m=2 n=1"
  //
  //  Thus, 3 delimiters are necessary:
  //
  const char *_delim    = " ";
  const char *__delim   = "=";
  const char *___delim  = ",";
  // and 3 (end) token placeholders
  char *_, *__, *___, *_end, *__end, *___end;
  //
  //  Setting ivariable gameinfo provides the interface with extra
  //  notifications when the start starts a game or simul or a game is
  //  observed. 
  //  
  //    "Note any new fields will be appended to the end so the
  //    interface must be able to handle this."
  //
  //  - <g1>  the string to identify a gameinfo line
  // 
  char *marker                      = strtok_r(cp,          _delim,     &_end);
  //
  // game_number
  unsigned int game_number          = atoi(strtok_r(NULL,   _delim,         &_end));
  //
  //
  // p=private(1/0)
  _                                 = strtok_r(NULL,        _delim,         &_end);     // e.g., "a=b"
  __                                = strtok_r(_,           __delim,        &__end);    // discard "p="
  bool private                      = atoi(strtok_r(NULL,   __delim,        &__end));
  //
  // t=type : one of {lightning, blitz, standard ... ?}
  _                                 = strtok_r(NULL,        _delim,         &_end);     
  __                                = strtok_r(_,           __delim,        &__end);    // discard "t="
  char * type                       = strtok_r(NULL,        __delim,        &__end);
  //
  // whether game is rated or unrated
  // r=rated(1/0)
  _                                 = strtok_r(NULL,        _delim,         &_end);     
  __                                = strtok_r(_,           __delim,        &__end);    // discard "r="
  bool rated                        = atoi(strtok_r(NULL,   __delim,        &__end));
  //
  // u=white_registered(1/0),black_registered(1/0) 
  _                                 = strtok_r(NULL,        _delim,         &_end);     
  __                                = strtok_r(_,           __delim,        &__end);    // discard "u="
  ___                               = strtok_r(NULL,        __delim,        &__end);    // "m,n"
  bool white_registered             = atoi(strtok_r(___,    ___delim,       &___end));  // m
  bool black_registered             = atoi(strtok_r(NULL,   ___delim,       &___end));  // n
  //
  // it=initial_white_time,initial_black_time
  _                                 = strtok_r(NULL,        _delim,         &_end);      
  __                                = strtok_r(_,           __delim,        &__end);    // discard "it="
  ___                               = strtok_r(NULL,        __delim,        &__end);    // "m,n"
  unsigned int white_initial_time   = atoi(strtok_r(___,    ___delim,       &___end));
  unsigned int black_initial_time   = atoi(strtok_r(NULL,   ___delim,       &___end));
  //
  // 
  // i=initial_white_inc, initial_black_inc
  _                                 = strtok_r(NULL,        _delim,         &_end);      
  __                                = strtok_r(_,           __delim,        &__end);    // discard "i="
  ___                               = strtok_r(NULL,        __delim,        &__end);
  unsigned white_increment          = atoi(strtok_r(___,    ___delim,       &___end));
  unsigned black_increment          = atoi(strtok_r(NULL,   ___delim,       &___end));
  //
  // pt=partner's_game_number(or 0 if none)
  _                                 = strtok_r(NULL,        _delim,         &_end);      
  __                                = strtok_r(_,           __delim,        &__end);    // discard "pt="
  unsigned int partner_game_number  = atoi(strtok_r(NULL,   __delim,        &__end));
  //
  // rt=white_rating(+ provshow character),black_rating(+ provshow chara    cter)
  _                                 = strtok_r(NULL,        _delim,         &_end);      
  __                                = strtok_r(_,           __delim,        &__end);    // discard "rt="
  ___                               = strtok_r(NULL,        __delim,        &__end);
  char * white_rating               = strtok_r(___,         ___delim,       &___end);
  char * black_rating               = strtok_r(NULL,        ___delim,       &___end);
  //
  // ts=white_uses_timeseal(0/1),black_uses_timeseal(0/1)
  _                                 = strtok_r(NULL,        _delim,         &_end);      
  __                                = strtok_r(_,           __delim,        &__end);    // discard "ts="
  ___                               = strtok_r(NULL,        __delim,        &__end);
  bool white_timeseal               = atoi(strtok_r(___,    ___delim,       &___end));
  bool black_timeseal               = atoi(strtok_r(NULL,   ___delim,       &___end));

  /* end parsing */

  /* begin assignment */

  //                            mark unused to suppress compiler warnings
  UNUSED( _                     );
  UNUSED( __                    );
  UNUSED( ___                   );
  UNUSED( marker                );
  UNUSED( private               );
  UNUSED( black_increment       );
  UNUSED( white_increment       );
  UNUSED( black_registered      );
  UNUSED( white_registered      );
  UNUSED( black_initial_time    );
  UNUSED( white_initial_time    );
  UNUSED( partner_game_number   );
  
  u->text                       = strdup(line);
  u->type                       = strdup( type );
  u->type_sym                   = type_to_sym ( type );
  u->rated                      = rated;
  u->game_number                = game_number;
  u->white_rating               = strdup( white_rating );
  u->black_rating               = strdup( black_rating );
  u->white_timeseal             = white_timeseal;
  u->black_timeseal             = black_timeseal;

  /* end assignment */

  //return gi;
}


/// Logging

void print_g1(UPDATE *update)
{
  debug("\
          text: %s\n \
          game_number: %i\n \
          type: %s\n \
          rated: %i\n \
          white_rating: %s \n \
          black_rating: %s \n \
          white_timeseal: %i \n \
          black_timeseal: %i \n",
          update->text,
          update->game_number,
          update->type,
          update->rated,
          update->white_rating,
          update->black_rating,
          update->white_timeseal,
          update->black_timeseal
  );
}

void print_s12(UPDATE *update)
{
  debug("\
          text: %s\n \
          my_turn: %i\n \
          i_can_castle_short: %i\n \
          i_can_castle_long: %i\n \
          opp_can_castle_short: %i\n \
          opp_can_castle_long: %i\n \
          my_nick: %s\n \
          opp_nick: %s\n \
          my_status: %i\n \
          match_minutes: %i\n \
          match_increment: %i\n \
          my_strength: %i\n \
          opp_strength: %i\n \
          my_seconds: %i\n \
          opp_seconds: %i\n",
          update->text,
          update->my_turn,
          update->i_can_castle_short,
          update->i_can_castle_long,
          update->opp_can_castle_short,
          update->opp_can_castle_long,
          update->my_nick,
          update->opp_nick,
          update->my_status,
          update->match_minutes,
          update->match_increment,
          update->my_strength,
          update->opp_strength,
          update->my_ms,
          update->opp_ms
  );
  for (int i=0; i<N_COLS; i++)
    for (int j=0; j<N_ROWS; j++)
      debug(" %i %i %s ", i, j, update->board[i][j]);
    debug("\n");
}

