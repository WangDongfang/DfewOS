/* vi: set sw=4 ts=4: */
/*
 * tiny vi.c: A small 'vi' clone
 * Copyright (C) 2000, 2001 Sterling Huxley <sterling@europa.com>
 *
 * Licensed under the GPL v2 or later, see the file LICENSE in this tarball.
 */

/*
 * Things To Do:
 * Simple It.
 */

#include "vi_fix.h"

/* 0x9b is Meta-ESC */
#define Isprint(c) ((unsigned char)(c) >= ' ' && (unsigned char)(c) < 0x7f)

enum {
	MAX_TABSTOP = 32, // sanity limit
	// User input len. Need not be extra big.
	// Lines in file being edited *can* be bigger than this.
	MAX_INPUT_LEN = 128,
	// Sanity limits. We have only one buffer of this size.
	MAX_SCR_COLS = CONFIG_FEATURE_VI_MAX_LEN,
	MAX_SCR_ROWS = CONFIG_FEATURE_VI_MAX_LEN,
};

// Misc. non-Ascii keys that report an escape sequence
#define VI_K_UP			(char)128	// cursor key Up
#define VI_K_DOWN		(char)129	// cursor key Down
#define VI_K_RIGHT		(char)130	// Cursor Key Right
#define VI_K_LEFT		(char)131	// cursor key Left
#define VI_K_HOME		(char)132	// Cursor Key Home
#define VI_K_END		(char)133	// Cursor Key End
#define VI_K_INSERT		(char)134	// Cursor Key Insert
#define VI_K_DELETE		(char)135	// Cursor Key Insert
#define VI_K_PAGEUP		(char)136	// Cursor Key Page Up
#define VI_K_PAGEDOWN	(char)137	// Cursor Key Page Down

#define VI_K_FUN1		(char)138	// Function Key F1
#define VI_K_FUN2		(char)139	// Function Key F2
#define VI_K_FUN3		(char)140	// Function Key F3
#define VI_K_FUN4		(char)141	// Function Key F4
#define VI_K_FUN5		(char)142	// Function Key F5
#define VI_K_FUN6		(char)143	// Function Key F6
#define VI_K_FUN7		(char)144	// Function Key F7
#define VI_K_FUN8		(char)145	// Function Key F8
#define VI_K_FUN9		(char)146	// Function Key F9
#define VI_K_FUN10		(char)147	// Function Key F10
#define VI_K_FUN11		(char)148	// Function Key F11
#define VI_K_FUN12		(char)149	// Function Key F12

/* vt102 typical ESC sequence */
/* terminal standout start/normal ESC sequence */
static const char SOs[] = "\033[7m";
static const char SOn[] = "\033[0m";
static const char bell[] = "\007";/* terminal bell sequence */
static const char Ceol[] = "\033[0K";/* Clear-end-of-line and Clear-end-of-screen ESC sequence */
static const char Ceos[] = "\033[0J";
static const char CMrc[] = "\033[%d;%dH";/* Cursor motion arbitrary destination ESC sequence */
static const char CMup[] = "\033[A";/* Cursor motion up and down ESC sequence */
static const char CMdown[] = "\n";

enum {
	YANKONLY = FALSE,
	YANKDEL = TRUE,
	FORWARD = 1,	// code depends on "1"  for array index
	BACK = -1,	// code depends on "-1" for array index
	LIMITED = 0,	// how much of _G_pc_text[] in char_search
	FULL = 1,	// how much of _G_pc_text[] in char_search

	S_BEFORE_WS = 1,	// used in skip_thing() for moving "dot"
	S_TO_WS = 2,		// used in skip_thing() for moving "dot"
	S_OVER_WS = 3,		// used in skip_thing() for moving "dot"
	S_END_PUNCT = 4,	// used in skip_thing() for moving "dot"
	S_END_ALNUM = 5,	// used in skip_thing() for moving "dot"
};

/* vi.c expects chars to be unsigned. */
/* busybox build system provides that, but it's better */
/* to audit and fix the source */

static smallint _G_vi_setops;
#define VI_AUTOINDENT 1
#define VI_SHOWMATCH  2
#define VI_IGNORECASE 4
#define VI_ERR_METHOD 8
#define autoindent (_G_vi_setops & VI_AUTOINDENT)
#define showmatch  (_G_vi_setops & VI_SHOWMATCH )
#define ignorecase (_G_vi_setops & VI_IGNORECASE)
#define err_method (_G_vi_setops & VI_ERR_METHOD)/* indicate error with beep or flash */

/**********************************************************************************************************
  Global Variable
**********************************************************************************************************/
static smallint _G_editing;          // >0 while we are editing a file
                                     // [code audit says "can be 0 or 1 only"]
static smallint _G_c_md_mode;        // 0=command  1=insert 2=replace
static smallint _G_file_modified;    // buffer contents changed
static smallint _G_last_file_modified = -1;

static int _G_i_fn_start;            // index of first cmd line file name
static int _G_i_save_argc;           // how many file names on cmd line
static int _G_i_cmdcnt;              // repetition count
static int _G_i_rows, _G_i_columns;  // the terminal screen is this size
static int _G_i_crow, _G_i_ccol;     // cursor is on Crow x Ccol
static int _G_i_offset;              // chars scrolled off the screen to the left

#define STATUS_BUFFER_LEN  200
static char *_G_pc_status_buffer;    // mesages to the user
static int _G_i_have_status_msg;     // is default edit status needed?
                                     // [don't make smallint!]
static int _G_i_last_status_cksum;   // hash of current status line

static char *_G_pc_cmd_line;         // the last command line buffer
static char *_G_pc_current_filename; // current file name
static char *_G_pc_screen;           // pointer to the virtual screen buffer
static int   _G_i_screensize;        // and its size
static char *_G_pc_screenbegin;      // index into _G_pc_text[], of top line on the screen
static int   _G_i_tabstop;
static char  _G_c_erase_char;        // the users erase character
static char  _G_c_last_input_char;   // last char read from user
static char  _G_c_last_forward_char; // last char searched for with 'f'

/* Moving biggest data to malloced space... */
struct globals {
	/* many references - keep near the top of globals */
	char *text, *end;       // pointers to the user data in memory
	char *dot;              // where all the action takes place
	int   text_size;		// size of the allocated buffer

	// Should be just enough to hold a key sequence,
	// but CRASME mode uses it as generated command buffer too
	char  readbuffer[32];

	char  scr_out_buf[MAX_SCR_COLS + MAX_TABSTOP * 2];
};

/**************************** HANHUI ***************************/
struct globals  *ptr_to_globals;
/**************************** HANHUI ***************************/
#define G (*ptr_to_globals)
#define _GG_pc_text           (G.text          )
#define _GG_i_text_size       (G.text_size     )
#define _GG_pc_end            (G.end           )
#define _GG_pc_dot            (G.dot           )
#define _GG_pc_readbuffer     (G.readbuffer    )
#define _GG_pc_scr_out_buf    (G.scr_out_buf   )

#define INIT_G() do { \
	SET_PTR_TO_GLOBALS(xzalloc(sizeof(G))); \
} while (0)
#define DEINIT_G() do { \
    free(ptr_to_globals);\
    ptr_to_globals = NULL;\
} while (0)

/**********************************************************************************************************
  functions declear
**********************************************************************************************************/
static int  init_text_buffer(char *); // init from file or create new
static void edit_file(char *);	// edit one file
static void do_cmd(char);	// execute a command
static int  next_tabstop(int);
static void sync_cursor(char *, int *, int *);	// synchronize the screen cursor to dot
static char *begin_line(char *);	// return pointer to cur line B-o-l
static char *end_line(char *);	// return pointer to cur line E-o-l
static char *prev_line(char *);	// return pointer to prev line B-o-l
static char *next_line(char *);	// return pointer to next line B-o-l
static char *end_screen(void);	// get pointer to last char on screen
static int  count_lines(char *, char *);	// count line from start to stop
static char *find_line(int);	// find begining of line #li
static char *move_to_col(char *, int);	// move "p" to column l
static void dot_left(void);	// move dot left- dont leave line
static void dot_right(void);	// move dot right- dont leave line
static void dot_begin(void);	// move dot to B-o-l
static void dot_end(void);	// move dot to E-o-l
static void dot_next(void);	// move dot to next line B-o-l
static void dot_prev(void);	// move dot to prev line B-o-l
static void dot_scroll(int, int);	// move the screen up or down
static void dot_skip_over_ws(void);	// move dot pat WS
static void dot_delete(void);	// delete the char at 'dot'
static char *bound_dot(char *);	// make sure  text[0] <= P < "end"
static char *new_screen(int, int);	// malloc virtual screen memory
static char *char_insert(char *, char);	// insert the char c at 'p'
static char *stupid_insert(char *, char);	// stupidly insert the char c at 'p'
static int  find_range(char **, char **, char);	// return pointers for an object
static int  st_test(char *, int, int, char *);	// helper for skip_thing()
static char *skip_thing(char *, int, int, int);	// skip some object
static char *find_pair(char *, char);	// find matching pair ()  []  {}
static char *text_hole_delete(char *, char *);	// at "p", delete a 'size' byte hole
static char *text_hole_make(char *, int);	// at "p", make a 'size' byte hole
static char *yank_delete(char *, char *, int, int);	// yank _G_pc_text[] into register then delete
static void show_help(void);	// display some help info
static void rawmode(void);	// set "raw" mode on tty
static void cookmode(void);	// return to "cooked" mode on tty
static int  mysleep(int);// sleep for 'h' 1/100 seconds, return 1/0 if stdin is (ready for read)/(not ready)
static char readit(void);	// read (maybe cursor) key from stdin
static char get_one_char(void);	// read 1 char from stdin
static int  file_size(const char *);   // what is the byte size of "fn"
static int  file_insert(const char *, char *);
static int  file_write(char *, char *, char *);
static void place_cursor(int, int, int);
static void screen_erase(void);
static void clear_to_eol(void);
static void clear_to_eos(void);
static void standout_start(void);	// send "start reverse video" sequence
static void standout_end(void);	// send "end reverse video" sequence
static void flash(int);		// flash the terminal screen
static void show_status_line(void);	// put a message on the bottom line
static void status_line(const char *, ...);     // print to status buf
static void status_line_bold(const char *, ...);
static void not_implemented(const char *); // display "Not implemented" message
static int  format_edit_status(void);	// format file status on status line
static void redraw(int);	// force a full screen refresh
static char *format_line(char* /*, int*/);
static void refresh(int);	// update the terminal from _G_pc_screen[]

static void Indicate_Error(void);       // use flash or beep to indicate error
static void Hit_Return(void);
static void Free_File_Mem();

#define indicate_error(c) Indicate_Error()

static void write1(const char *out)
{
	fputs(out, stdout);
}

int vi_main(int argc, char **argv)
{
#ifdef DFEW_OS
    int i_file = 1; /* start of file name in argv */
#else /* DFEW_OS */
	int c;
    GETOPT getoptState;
#endif /* DFEW_OS */
	RESERVE_CONFIG_BUFFER(STATUS_BUFFER, STATUS_BUFFER_LEN);

	INIT_G();

	_G_pc_status_buffer = STATUS_BUFFER;
	_G_i_last_status_cksum = 0;
	_GG_pc_text = NULL;

	_G_vi_setops = VI_AUTOINDENT | VI_SHOWMATCH | VI_IGNORECASE;

	//  1-  process $HOME/.exrc file (not inplemented yet)
	//  2-  process EXINIT variable from environment
	//  3-  process command line args

#ifdef DFEW_OS
    if ((argc >= 2) && (strcmp(argv[1], "-h") == 0)) {
        show_help();
        DEINIT_G();
        return 1;
    }
	_G_i_fn_start = i_file;	// remember first file name for :next and :rew
	_G_i_save_argc = argc;
    if (i_file >= argc) {
        edit_file(DEFAULT_FILE_NAME);
    } else {
        for (; i_file < argc; i_file++) {
            edit_file(argv[i_file]);
        }
    }
#else /* DFEW_OS */
    getoptInit (&getoptState);
	while ((c = getopt_r(argc, argv, "hH?", &getoptState)) != -1) {
		switch (c) {
        case '?':
        case 'h':
		case 'H':
			show_help(); /* fall through */
		default:
            bb_show_usage();
            DEINIT_G();
			return 1;
		}
	}

	// The argv array can be used by the ":next"  and ":rewind" commands
	// save optind.
	_G_i_fn_start = getoptState.optind;	// remember first file name for :next and :rew
	_G_i_save_argc = argc;

	//----- This is the main file handling loop --------------
	if (getoptState.optind >= argc) {
		edit_file(DEFAULT_FILE_NAME);
	} else {
		for (; getoptState.optind < argc; getoptState.optind++) {
			edit_file(argv[getoptState.optind]);
		}
	}
	//-----------------------------------------------------------
#endif /* DFEW_OS */

    puts("\x1b[2J");
    puts("\x1b[1;1H");
    
    DEINIT_G();

	return 0;
}

/* read text from file or create an empty buf */
/* will also update _G_pc_current_filename */
static int init_text_buffer(char *fn)
{
	int rc;
	int size = file_size(fn);	// file size. -1 means does not exist.

	/* allocate/reallocate text buffer */
	_GG_i_text_size = size * 2;
	if (_GG_i_text_size < 10240)
		_GG_i_text_size = 10240;	// have a minimum size for new files
	_G_pc_screenbegin = _GG_pc_dot = _GG_pc_end = _GG_pc_text = xzalloc(_GG_i_text_size);

	if (fn != _G_pc_current_filename) {
		_G_pc_current_filename = xstrdup(fn);
	}
	if (size < 0) {
		// file dont exist. Start empty buf with dummy line
		char_insert(_GG_pc_text, '\n');
		rc = 0;
	} else {
		rc = file_insert(fn, _GG_pc_text);
			
	}
	_G_file_modified = 0;
	_G_last_file_modified = -1;

    // alloc last command line buffer
    _G_pc_cmd_line = xmalloc(MAX_INPUT_LEN);

	return rc;
}

static void edit_file(char *fn)
{
	char c = '\0';
	int size;

	_G_editing = 1;	// 0 = exit, 1 = one file, 2 = multiple files
	rawmode();
	_G_i_rows = 24;
	_G_i_columns = 80;
	size = 0;

	new_screen(_G_i_rows, _G_i_columns);	// get memory for virtual screen
	init_text_buffer(fn);

	_G_c_last_forward_char = _G_c_last_input_char = '\0';
	_G_i_crow = 0;
	_G_i_ccol = 0;

	_G_c_md_mode = 0;		// 0=command  1=insert  2='R'eplace
	_G_i_cmdcnt = 0;
	_G_i_tabstop = 8;
	_G_i_offset = 0;			// no horizontal offset

	redraw(FALSE);			// dont force every col re-draw
	//------This is the main Vi cmd handling loop -----------------------
	while (_G_editing > 0) {

		_G_c_last_input_char = c = get_one_char();	// get a cmd from user

		do_cmd(c);		// execute the user command

		//
		// poll to see if there is input already waiting. if we are
		// not able to display output fast enough to keep up, skip
		// the display update until we catch up with input.
		if (mysleep(0) == 0) {
			// no input pending- so update output
			refresh(FALSE);
			show_status_line();
		}
	}
	//-------------------------------------------------------------------
	place_cursor(_G_i_rows, 0, FALSE);	// go to bottom of screen
	clear_to_eol();		// Erase to end of line
	cookmode();

    // free memory
    Free_File_Mem();
}

//----- Free The edit_file() Alloced Memory -----------------------------------
static void Free_File_Mem()
{
    free(_G_pc_current_filename);
    _G_pc_current_filename = NULL;

    free(_GG_pc_text);
    _GG_pc_text = NULL;

    free(_G_pc_screen);
    _G_pc_screen = NULL;

    free(_G_pc_cmd_line);
    _G_pc_cmd_line = NULL;
}

//----- The Colon commands -------------------------------------
static void Hit_Return(void)
{
	char c;

	standout_start();
	write1("[Hit return to continue]");
	standout_end();
	while ((c = get_one_char()) != '\n' && c != '\r')
		continue;
	redraw(TRUE);		// force redraw all
}

static int next_tabstop(int col)
{
	return col + ((_G_i_tabstop - 1) - (col % _G_i_tabstop));
}

//----- Synchronize the cursor to Dot --------------------------
static void sync_cursor(char *d, int *row, int *col)
{
	char *beg_cur;	// begin and end of "d" line
	char *tp;
	int cnt, ro, co;

	beg_cur = begin_line(d);	// first char of cur line

	if (beg_cur < _G_pc_screenbegin) {
		// "d" is before top line on screen
		// how many lines do we have to move
		cnt = count_lines(beg_cur, _G_pc_screenbegin);
sc1:
		_G_pc_screenbegin = beg_cur;
		if (cnt > (_G_i_rows - 1) / 2) {
			// we moved too many lines. put "dot" in middle of screen
			for (cnt = 0; cnt < (_G_i_rows - 1) / 2; cnt++) {
				_G_pc_screenbegin = prev_line(_G_pc_screenbegin);
			}
		}
	} else {
		char *end_scr;	// begin and end of screen
		end_scr = end_screen();	// last char of screen
		if (beg_cur > end_scr) {
			// "d" is after bottom line on screen
			// how many lines do we have to move
			cnt = count_lines(end_scr, beg_cur);
			if (cnt > (_G_i_rows - 1) / 2)
				goto sc1;	// too many lines
			for (ro = 0; ro < cnt - 1; ro++) {
				// move screen begin the same amount
				_G_pc_screenbegin = next_line(_G_pc_screenbegin);
				// now, move the end of screen
				end_scr = next_line(end_scr);
				end_scr = end_line(end_scr);
			}
		}
	}
	// "d" is on screen- find out which row
	tp = _G_pc_screenbegin;
	for (ro = 0; ro < _G_i_rows - 1; ro++) {	// drive "ro" to correct row
		if (tp == beg_cur)
			break;
		tp = next_line(tp);
	}

	// find out what col "d" is on
	co = 0;
	while (tp < d) { // drive "co" to correct column
		if (*tp == '\n') //vda || *tp == '\0')
			break;
		if (*tp == '\t') {
			// handle tabs like real vi
			if (d == tp && _G_c_md_mode) {
				break;
			} else {
				co = next_tabstop(co);
			}
		} else if ((unsigned char)*tp < ' ' || *tp == 0x7f) {
			co++; // display as ^X, use 2 columns
		}
		co++;
		tp++;
	}

	// "co" is the column where "dot" is.
	// The screen has "_G_i_columns" columns.
	// The currently displayed columns are  0+offset -- columns+ofset
	// |-------------------------------------------------------------|
	//               ^ ^                                ^
	//        offset | |------- columns ----------------|
	//
	// If "co" is already in this range then we do not have to adjust offset
	//      but, we do have to subtract the "_G_i_offset" bias from "co".
	// If "co" is outside this range then we have to change "_G_i_offset".
	// If the first char of a line is a tab the cursor will try to stay
	//  in column 7, but we have to set _G_i_offset to 0.

	if (co < 0 + _G_i_offset) {
		_G_i_offset = co;
	}
	if (co >= _G_i_columns + _G_i_offset) {
		_G_i_offset = co - _G_i_columns + 1;
	}
	// if the first char of the line is a tab, and "dot" is sitting on it
	//  force _G_i_offset to 0.
	if (d == beg_cur && *d == '\t') {
		_G_i_offset = 0;
	}
	co -= _G_i_offset;

	*row = ro;
	*col = co;
}

//----- Text Movement Routines ---------------------------------
static char *begin_line(char *p) // return pointer to first char cur line
{
	if (p > _GG_pc_text) {
		p = memrchr(_GG_pc_text, '\n', p - _GG_pc_text);
		if (!p)
			return _GG_pc_text;
		return p + 1;
	}
	return p;
}

static char *end_line(char *p) // return pointer to NL of cur line line
{
	if (p < _GG_pc_end - 1) {
		p = memchr(p, '\n', _GG_pc_end - p - 1);
		if (!p)
			return _GG_pc_end - 1;
	}
	return p;
}

static char *dollar_line(char *p) // return pointer to just before NL line
{
	p = end_line(p);
	// Try to stay off of the Newline
	if (*p == '\n' && (p - begin_line(p)) > 0)
		p--;
	return p;
}

static char *prev_line(char *p) // return pointer first char prev line
{
	p = begin_line(p);	// goto begining of cur line
	if (p[-1] == '\n' && p > _GG_pc_text)
		p--;			// step to prev line
	p = begin_line(p);	// goto begining of prev line
	return p;
}

static char *next_line(char *p) // return pointer first char next line
{
	p = end_line(p);
	if (*p == '\n' && p < _GG_pc_end - 1)
		p++;			// step to next line
	return p;
}

//----- Text Information Routines ------------------------------
static char *end_screen(void)
{
	char *q;
	int cnt;

	// find new bottom line
	q = _G_pc_screenbegin;
	for (cnt = 0; cnt < _G_i_rows - 2; cnt++)
		q = next_line(q);
	q = end_line(q);
	return q;
}

// count line from start to stop
static int count_lines(char *start, char *stop)
{
	char *q;
	int cnt;

	if (stop < start) { // start and stop are backwards- reverse them
		q = start;
		start = stop;
		stop = q;
	}
	cnt = 0;
	stop = end_line(stop);
	while (start <= stop && start <= _GG_pc_end - 1) {
		start = end_line(start);
		if (*start == '\n')
			cnt++;
		start++;
	}
	return cnt;
}

static char *find_line(int li)	// find begining of line #li
{
	char *q;

	for (q = _GG_pc_text; li > 1; li--) {
		q = next_line(q);
	}
	return q;
}

//----- Dot Movement Routines ----------------------------------
static void dot_left(void)
{
	if (_GG_pc_dot > _GG_pc_text && _GG_pc_dot[-1] != '\n')
		_GG_pc_dot--;
}

static void dot_right(void)
{
	if (_GG_pc_dot < _GG_pc_end - 1 && *_GG_pc_dot != '\n')
		_GG_pc_dot++;
}

static void dot_begin(void)
{
	_GG_pc_dot = begin_line(_GG_pc_dot);	// return pointer to first char cur line
}

static void dot_end(void)
{
	_GG_pc_dot = end_line(_GG_pc_dot);	// return pointer to last char cur line
}

static char *move_to_col(char *p, int l)
{
	int co;

	p = begin_line(p);
	co = 0;
	while (co < l && p < _GG_pc_end) {
		if (*p == '\n') //vda || *p == '\0')
			break;
		if (*p == '\t') {
			co = next_tabstop(co);
		} else if (*p < ' ' || *p == 127) {
			co++; // display as ^X, use 2 columns
		}
		co++;
		p++;
	}
	return p;
}

static void dot_next(void)
{
	_GG_pc_dot = next_line(_GG_pc_dot);
}

static void dot_prev(void)
{
	_GG_pc_dot = prev_line(_GG_pc_dot);
}

static void dot_scroll(int cnt, int dir)
{
	char *q;

	for (; cnt > 0; cnt--) {
		if (dir < 0) {
			// scroll Backwards
			// ctrl-Y scroll up one line
			_G_pc_screenbegin = prev_line(_G_pc_screenbegin);
		} else {
			// scroll Forwards
			// ctrl-E scroll down one line
			_G_pc_screenbegin = next_line(_G_pc_screenbegin);
		}
	}
	// make sure "_GG_pc_dot" stays on the screen so we dont scroll off
	if (_GG_pc_dot < _G_pc_screenbegin)
		_GG_pc_dot = _G_pc_screenbegin;
	q = end_screen();	// find new bottom line
	if (_GG_pc_dot > q)
		_GG_pc_dot = begin_line(q);	// is _GG_pc_dot is below bottom line?
	dot_skip_over_ws();
}

static void dot_skip_over_ws(void)
{
	// skip WS
	while (isspace(*_GG_pc_dot) && *_GG_pc_dot != '\n' && _GG_pc_dot < _GG_pc_end - 1)
		_GG_pc_dot++;
}

static void dot_delete(void)	// delete the char at '_GG_pc_dot'
{
	text_hole_delete(_GG_pc_dot, _GG_pc_dot);
}

static char *bound_dot(char *p) // make sure  text[0] <= P < "end"
{
	if (p >= _GG_pc_end && _GG_pc_end > _GG_pc_text) {
		p = _GG_pc_end - 1;
		indicate_error('1');
	}
	if (p < _GG_pc_text) {
		p = _GG_pc_text;
		indicate_error('2');
	}
	return p;
}

//----- Helper Utility Routines --------------------------------

//----------------------------------------------------------------
//----- Char Routines --------------------------------------------
/* Chars that are part of a word-
 *    0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
 * Chars that are Not part of a word (stoppers)
 *    !"#$%&'()*+,-./:;<=>?@[\]^`{|}~
 * Chars that are WhiteSpace
 *    TAB NEWLINE VT FF RETURN SPACE
 * DO NOT COUNT NEWLINE AS WHITESPACE
 */

static char *new_screen(int ro, int co)
{
	int li;

	_G_i_screensize = ro * co + 8;
	_G_pc_screen = xmalloc(_G_i_screensize);
	// initialize the new screen. assume this will be a empty file.
	screen_erase();
	//   non-existent _G_pc_text[] lines start with a tilde (~).
	for (li = 1; li < ro - 1; li++) {
		_G_pc_screen[(li * co) + 0] = '~';
	}
	return _G_pc_screen;
}

static char *char_insert(char * p, char c) // insert the char c at 'p'
{
	if (c == 22) {		// Is this an ctrl-V?
		p = stupid_insert(p, '^');	// use ^ to indicate literal next
		p--;			// backup onto ^
		refresh(FALSE);	// show the ^
		c = get_one_char();
		*p = c;
		p++;
		_G_file_modified++;	// has the file been modified
	} else if (c == 27) {	// Is this an ESC?
		_G_c_md_mode = 0;
		_G_i_cmdcnt = 0;
		_G_i_last_status_cksum = 0;	// force status update
		if ((p[-1] != '\n') && (_GG_pc_dot > _GG_pc_text)) {
			p--;
		}
	} else if (c == _G_c_erase_char || c == 8 || c == 127) { // Is this a BS
		//     123456789
		if ((p[-1] != '\n') && (_GG_pc_dot>_GG_pc_text)) {
			p--;
			p = text_hole_delete(p, p);	// shrink buffer 1 char
		}
	} else {
		// insert a char into _G_pc_text[]
		char *sp;		// "save p"

		if (c == 13)
			c = '\n';	// translate \r to \n
		sp = p;			// remember addr of insert
		p = stupid_insert(p, c);	// insert the char

	}
	return p;
}

static char *stupid_insert(char * p, char c) // stupidly insert the char c at 'p'
{
	p = text_hole_make(p, 1);
	if (p != 0) {
		*p = c;
		_G_file_modified++;	// has the file been modified
		p++;
	}
	return p;
}

static int find_range(char ** start, char ** stop, char c)
{
	char *save_dot, *p, *q, *t;
	int cnt, multiline = 0;

	save_dot = _GG_pc_dot;
	p = q = _GG_pc_dot;

	if (strchr("cdy><", c)) {
		// these cmds operate on whole lines
		p = q = begin_line(p);
		for (cnt = 1; cnt < _G_i_cmdcnt; cnt++) {
			q = next_line(q);
		}
		q = end_line(q);
	} else if (strchr("^%$0bBeEfth\b\177", c)) {
		// These cmds operate on char positions
		do_cmd(c);		// execute movement cmd
		q = _GG_pc_dot;
	} else if (strchr("wW", c)) {
		do_cmd(c);		// execute movement cmd
		// if we are at the next word's first char
		// step back one char
		// but check the possibilities when it is true
		if (_GG_pc_dot > _GG_pc_text && ((isspace(_GG_pc_dot[-1]) && !isspace(_GG_pc_dot[0]))
				|| (ispunct(_GG_pc_dot[-1]) && !ispunct(_GG_pc_dot[0]))
				|| (isalnum(_GG_pc_dot[-1]) && !isalnum(_GG_pc_dot[0]))))
			_GG_pc_dot--;		// move back off of next word
		if (_GG_pc_dot > _GG_pc_text && *_GG_pc_dot == '\n')
			_GG_pc_dot--;		// stay off NL
		q = _GG_pc_dot;
	} else if (strchr("H-k{", c)) {
		// these operate on multi-lines backwards
		q = end_line(_GG_pc_dot);	// find NL
		do_cmd(c);		// execute movement cmd
		dot_begin();
		p = _GG_pc_dot;
	} else if (strchr("L+j}\r\n", c)) {
		// these operate on multi-lines forwards
		p = begin_line(_GG_pc_dot);
		do_cmd(c);		// execute movement cmd
		dot_end();		// find NL
		q = _GG_pc_dot;
	} else {
	    // nothing -- this causes any other values of c to
	    // represent the one-character range under the
	    // cursor.  this is correct for ' ' and 'l', but
	    // perhaps no others.
	    //
	}
	if (q < p) {
		t = q;
		q = p;
		p = t;
	}

	// backward char movements don't include start position 
	if (q > p && strchr("^0bBh\b\177", c)) q--;

	multiline = 0;
	for (t = p; t <= q; t++) {
		if (*t == '\n') {
			multiline = 1;
			break;
		}
	}

	*start = p;
	*stop = q;
	_GG_pc_dot = save_dot;
	return multiline;
}

static int st_test(char * p, int type, int dir, char * tested)
{
	char c, c0, ci;
	int test, inc;

	inc = dir;
	c = c0 = p[0];
	ci = p[inc];
	test = 0;

	if (type == S_BEFORE_WS) {
		c = ci;
		test = ((!isspace(c)) || c == '\n');
	}
	if (type == S_TO_WS) {
		c = c0;
		test = ((!isspace(c)) || c == '\n');
	}
	if (type == S_OVER_WS) {
		c = c0;
		test = ((isspace(c)));
	}
	if (type == S_END_PUNCT) {
		c = ci;
		test = ((ispunct(c)));
	}
	if (type == S_END_ALNUM) {
		c = ci;
		test = ((isalnum(c)) || c == '_');
	}
	*tested = c;
	return test;
}

static char *skip_thing(char * p, int linecnt, int dir, int type)
{
	char c;

	while (st_test(p, type, dir, &c)) {
		// make sure we limit search to correct number of lines
		if (c == '\n' && --linecnt < 1)
			break;
		if (dir >= 0 && p >= _GG_pc_end - 1)
			break;
		if (dir < 0 && p <= _GG_pc_text)
			break;
		p += dir;		// move to next char
	}
	return p;
}

// find matching char of pair  ()  []  {}
static char *find_pair(char * p, const char c)
{
	char match, *q;
	int dir, level;

	match = ')';
	level = 1;
	dir = 1;			// assume forward
	switch (c) {
	case '(': match = ')'; break;
	case '[': match = ']'; break;
	case '{': match = '}'; break;
	case ')': match = '('; dir = -1; break;
	case ']': match = '['; dir = -1; break;
	case '}': match = '{'; dir = -1; break;
	}
	for (q = p + dir; _GG_pc_text <= q && q < _GG_pc_end; q += dir) {
		// look for match, count levels of pairs  (( ))
		if (*q == c)
			level++;	// increase pair levels
		if (*q == match)
			level--;	// reduce pair level
		if (level == 0)
			break;		// found matching pair
	}
	if (level != 0)
		q = NULL;		// indicate no match
	return q;
}

//  open a hole in _G_pc_text[]
static char *text_hole_make(char * p, int size)	// at "p", make a 'size' byte hole
{
	char *src, *dest;
	int cnt;

	if (size <= 0)
		goto thm0;
	src = p;
	dest = p + size;
	cnt = _GG_pc_end - src;	// the rest of buffer
	if ( ((_GG_pc_end + size) >= (_GG_pc_text + _GG_i_text_size)) // TODO: realloc here
			|| memmove(dest, src, cnt) != dest) {
		status_line_bold("can't create room for new characters");
		p = NULL;
		goto thm0;
	}
	memset(p, ' ', size);	// clear new hole
	_GG_pc_end += size;		// adjust the new END
	_G_file_modified++;	// has the file been modified
thm0:
	return p;
}

//  close a hole in _G_pc_text[]
static char *text_hole_delete(char * p, char * q) // delete "p" through "q", inclusive
{
	char *src, *dest;
	int cnt, hole_size;

	// move forwards, from beginning
	// assume p <= q
	src = q + 1;
	dest = p;
	if (q < p) {		// they are backward- swap them
		src = p + 1;
		dest = q;
	}
	hole_size = q - p + 1;
	cnt = _GG_pc_end - src;
	if (src < _GG_pc_text || src > _GG_pc_end)
		goto thd0;
	if (dest < _GG_pc_text || dest >= _GG_pc_end)
		goto thd0;
	if (src >= _GG_pc_end)
		goto thd_atend;	// just delete the end of the buffer
	if (memmove(dest, src, cnt) != dest) {
		status_line_bold("can't delete the character");
	}
thd_atend:
	_GG_pc_end = _GG_pc_end - hole_size;	// adjust the new END
	if (dest >= _GG_pc_end)
		dest = _GG_pc_end - 1;	// make sure dest in below end-1
	if (_GG_pc_end <= _GG_pc_text)
		dest = _GG_pc_end = _GG_pc_text;	// keep pointers valid
	_G_file_modified++;	// has the file been modified
thd0:
	return dest;
}

// copy text into register, then delete text.
// if dist <= 0, do not include, or go past, a NewLine
//
static char *yank_delete(char * start, char * stop, int dist, int yf)
{
	char *p;

	// make sure start <= stop
	if (start > stop) {
		// they are backwards, reverse them
		p = start;
		start = stop;
		stop = p;
	}
	if (dist <= 0) {
		// we cannot cross NL boundaries
		p = start;
		if (*p == '\n')
			return p;
		// dont go past a NewLine
		for (; p + 1 <= stop; p++) {
			if (p[1] == '\n') {
				stop = p;	// "stop" just before NewLine
				break;
			}
		}
	}
	p = start;

	if (yf == YANKDEL) {
		p = text_hole_delete(start, stop);
	}					// delete lines
	return p;
}

static void show_help(void)
{
	puts("vi [-h]              : Show this tip\n");
	puts("vi file1 [file2 ...] : Edit file1, file2 ...");
}

//----- Set terminal attributes --------------------------------
static void rawmode(void)
{
#ifndef DFEW_OS
    ioctl(0, FIOSETOPTIONS, OPT_RAW);
    ioctl(1, FIOSETOPTIONS, OPT_RAW);
#endif /* DFEW_OS */
}

static void cookmode(void)
{
	fflush(stdout);

#ifndef DFEW_OS
	ioctl(0, FIOSETOPTIONS, OPT_TERMINAL);
    ioctl(1, FIOSETOPTIONS, OPT_TERMINAL);
#endif /* DFEW_OS */
}

//----- Come here when we get a window resize signal ---------
static int mysleep(int hund)	// sleep for 'h' 1/100 seconds
{
#ifdef DFEW_OS
    if (serial_tstc() != 0) {
        return 1;
    } else {
        return serial_pend(hund);
    }
#else /* DFEW_OS */
/**************************** HANHUI ***************************/
	struct timeval  tv = {0, 0};
	fd_set rfds;
	int    num;
	
	fflush(stdout);
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = hund * 10000;
	/////////////////////////////
	num = select(1, &rfds, NULL, NULL, &tv);
	/////////////////////////////
	return num;
/**************************** HANHUI ***************************/
#endif /* DFEW_OS */
}

static int chars_to_parse;

//----- IO Routines --------------------------------------------
static char readit(void)	// read (maybe cursor) key from stdin
{
	char c;
	int n;
	struct esc_cmds {
		const char seq[4];
		char val;
	};

	static const struct esc_cmds esccmds[] = {
		{"OA"  , VI_K_UP      },   // cursor key Up
		{"OB"  , VI_K_DOWN    },   // cursor key Down
		{"OC"  , VI_K_RIGHT   },   // Cursor Key Right
		{"OD"  , VI_K_LEFT    },   // cursor key Left
		{"OH"  , VI_K_HOME    },   // Cursor Key Home
		{"OF"  , VI_K_END     },   // Cursor Key End
		{"[A"  , VI_K_UP      },   // cursor key Up
		{"[B"  , VI_K_DOWN    },   // cursor key Down
		{"[C"  , VI_K_RIGHT   },   // Cursor Key Right
		{"[D"  , VI_K_LEFT    },   // cursor key Left
		{"[H"  , VI_K_HOME    },   // Cursor Key Home
		{"[F"  , VI_K_END     },   // Cursor Key End
		{"[1~" , VI_K_HOME    },   // Cursor Key Home
		{"[2~" , VI_K_INSERT  },   // Cursor Key Insert
		{"[3~" , VI_K_DELETE  },   // Cursor Key Delete
		{"[4~" , VI_K_END     },   // Cursor Key End
		{"[5~" , VI_K_PAGEUP  },   // Cursor Key Page Up
		{"[6~" , VI_K_PAGEDOWN},   // Cursor Key Page Down
		{"OP"  , VI_K_FUN1    },   // Function Key F1
		{"OQ"  , VI_K_FUN2    },   // Function Key F2
		{"OR"  , VI_K_FUN3    },   // Function Key F3
		{"OS"  , VI_K_FUN4    },   // Function Key F4
		// careful: these have no terminating NUL!
		{"[11~", VI_K_FUN1    },   // Function Key F1
		{"[12~", VI_K_FUN2    },   // Function Key F2
		{"[13~", VI_K_FUN3    },   // Function Key F3
		{"[14~", VI_K_FUN4    },   // Function Key F4
		{"[15~", VI_K_FUN5    },   // Function Key F5
		{"[17~", VI_K_FUN6    },   // Function Key F6
		{"[18~", VI_K_FUN7    },   // Function Key F7
		{"[19~", VI_K_FUN8    },   // Function Key F8
		{"[20~", VI_K_FUN9    },   // Function Key F9
		{"[21~", VI_K_FUN10   },   // Function Key F10
		{"[23~", VI_K_FUN11   },   // Function Key F11
		{"[24~", VI_K_FUN12   },   // Function Key F12
	};
	enum { ESCCMDS_COUNT = ARRAY_SIZE(esccmds) };

	fflush(stdout);
	n = chars_to_parse;
	// get input from User- are there already input chars in Q?
	if (n <= 0) {
		// the Q is empty, wait for a typed char
#ifdef DFEW_OS
        n = serial_read (_GG_pc_readbuffer, sizeof(_GG_pc_readbuffer));
#else /* DFEW_OS */
		n = safe_read(0, _GG_pc_readbuffer, sizeof(_GG_pc_readbuffer));
#endif /* DFEW_OS */
		if (n < 0) {
			if (errno == EBADF || errno == EFAULT || errno == EINVAL
			 || errno == EIO)
				_G_editing = 0; // want to exit
			errno = 0;
		}
		if (n <= 0)
			return 0;       // error

		if (_GG_pc_readbuffer[0] == 27) {
			// This is an ESC char. Is this Esc sequence?
			// Could be bare Esc key. See if there are any
			// more chars to read after the ESC. This would
			// be a Function or Cursor Key sequence.

#ifdef DFEW_OS
            while ((serial_tstc() != 0) && n <= (sizeof(_GG_pc_readbuffer) - 8)) {
				int r = serial_read(_GG_pc_readbuffer + n, sizeof(_GG_pc_readbuffer) - n);
#else /* DFEW_OS */
/**************************** HANHUI ***************************/
			struct timeval  t = {0, 0};
			fd_set          fdset;
			FD_ZERO(&fdset);
	        FD_SET(0, &fdset);
/**************************** HANHUI ***************************/
			// keep reading while there are input chars, and room in buffer
			// for a complete ESC sequence (assuming 8 chars is enough)
/**************************** HANHUI ***************************/
            while (select(1, &fdset, NULL, NULL, &t) > 0 && n <= (sizeof(_GG_pc_readbuffer) - 8)) {
/**************************** HANHUI ***************************/
				// read the rest of the ESC string
				int r = safe_read(0, _GG_pc_readbuffer + n, sizeof(_GG_pc_readbuffer) - n);
#endif /* DFEW_OS */
				if (r > 0)
					n += r;
			}
		}
		chars_to_parse = n;
	}
	c = _GG_pc_readbuffer[0];
	if (c == 27 && n > 1) {
		// Maybe cursor or function key?
		const struct esc_cmds *eindex;

		for (eindex = esccmds; eindex < &esccmds[ESCCMDS_COUNT]; eindex++) {
			int cnt = strnlen(eindex->seq, 4);
			if (n <= cnt)
				continue;
			if (strncmp(eindex->seq, _GG_pc_readbuffer + 1, cnt) != 0)
				continue;
			c = eindex->val; // magic char value
			n = cnt + 1; // squeeze out the ESC sequence
			goto found;
		}
		// defined ESC sequence not found
	}
	n = 1;
found:
	// remove key sequence from Q
	chars_to_parse -= n;
	memmove(_GG_pc_readbuffer, _GG_pc_readbuffer + n, sizeof(_GG_pc_readbuffer) - n);
	return c;
}

//----- IO Routines --------------------------------------------
static char get_one_char(void)
{
	char c;

	c = readit();		// get the users input

	return c;
}

// Get input line (uses "status line" area)
static char *get_input_line(const char *prompt)
{
	char c;
	int i;

	strcpy(_G_pc_cmd_line, prompt);
	_G_i_last_status_cksum = 0;	// force status update
	place_cursor(_G_i_rows - 1, 0, FALSE);	// go to Status line, bottom of screen
	clear_to_eol();		// clear the line
	write1(prompt);      // write out the :, /, or ? prompt

	i = strlen(_G_pc_cmd_line);
	while (i < MAX_INPUT_LEN) {
		c = get_one_char();
		if (c == '\n' || c == '\r' || c == 27)
			break;		// this is end of input
		if (c == _G_c_erase_char || c == 8 || c == 127) {
			// user wants to erase prev char
			_G_pc_cmd_line[--i] = '\0';
			write1("\b \b"); // erase char on screen
			if (i <= 0) // user backs up before b-o-l, exit
				break;
		} else {
			_G_pc_cmd_line[i] = c;
			_G_pc_cmd_line[++i] = '\0';
			bb_putchar(c);
		}
	}
	refresh(FALSE);
	return _G_pc_cmd_line;
}

static int file_size(const char *fn) // what is the byte size of "fn"
{
	struct stat st_buf;
	int cnt;

	cnt = -1;
#ifdef DFEW_OS
	if (fn && fn[0] && my_yaffs_stat(fn, &st_buf) == 0)	// see if file exists
#else /* DFEW_OS */
	if (fn && fn[0] && stat(fn, &st_buf) == 0)	// see if file exists
#endif /* DFEW_OS */
		cnt = (int) st_buf.st_size;
	return cnt;
}

static int file_insert(const char * fn, char *p)
{
	int cnt = -1;
	int fd, size;
	struct stat statbuf;

	/* Validate file */
#ifdef DFEW_OS
	if (my_yaffs_stat(fn, &statbuf) < 0) {
#else /* DFEW_OS */
	if (stat(fn, &statbuf) < 0) {
#endif /* DFEW_OS */
		status_line_bold("\"%s\" %s", fn, strerror(errno));
		goto fi0;
	}
	if ((statbuf.st_mode & S_IFREG) == 0) {
		// This is not a regular file
		status_line_bold("\"%s\" Not a regular file", fn);
		goto fi0;
	}
	if (p < _GG_pc_text || p > _GG_pc_end) {
		status_line_bold("Trying to insert file outside of memory");
		goto fi0;
	}

	// read file to buffer
	fd = open(fn, O_RDONLY, 0666);
	if (fd < 0) {
		status_line_bold("\"%s\" %s", fn, strerror(errno));
		goto fi0;
	}
	size = statbuf.st_size;
	p = text_hole_make(p, size);
	if (p == NULL)
		goto fi0;
	cnt = safe_read(fd, p, size);
	if (cnt < 0) {
		status_line_bold("\"%s\" %s", fn, strerror(errno));
		p = text_hole_delete(p, p + size - 1);	// un-do buffer insert
	} else if (cnt < size) {
		// There was a partial read, shrink unused space _GG_pc_text[]
		p = text_hole_delete(p + cnt, p + (size - cnt) - 1);	// un-do buffer insert
		status_line_bold("cannot read all of file \"%s\"", fn);
	}
	if (cnt >= size)
		_G_file_modified++;
	close(fd);

fi0:
	return cnt;
}

static int file_write(char * fn, char * first, char * last)
{
	int fd, cnt, charcnt;

	if (fn == 0) {
		status_line_bold("No current filename");
		return -2;
	}
	charcnt = 0;
	fd = open(fn, (O_WRONLY | O_CREAT | O_TRUNC), 0666);
	if (fd < 0)
		return -1;
	cnt = last - first + 1;
	charcnt = full_write(fd, first, cnt);
	if (charcnt == cnt) {
		// good write
		//_G_file_modified = FALSE; // the file has not been modified
	} else {
		charcnt = 0;
	}
	close(fd);
	return charcnt;
}

//----- Terminal Drawing ---------------------------------------
// The terminal is made up of '_G_i_rows' line of '_G_i_columns' columns.
// classically this would be 24 x 80.
//  screen coordinates
//  0,0     ...     0,79
//  1,0     ...     1,79
//  .       ...     .
//  .       ...     .
//  22,0    ...     22,79
//  23,0    ...     23,79   <- status line

//----- Move the cursor to row x col (count from 0, not 1) -------
static void place_cursor(int row, int col, int optimize)
{
	char cm1[sizeof(CMrc) + sizeof(int)*3 * 2];
	char *cm;

	if (row < 0) row = 0;
	if (row >= _G_i_rows) row = _G_i_rows - 1;
	if (col < 0) col = 0;
	if (col >= _G_i_columns) col = _G_i_columns - 1;

	//----- 1.  Try the standard terminal ESC sequence
	sprintf(cm1, CMrc, row + 1, col + 1);
	cm = cm1;

	write1(cm);
}

//----- Erase from cursor to end of line -----------------------
static void clear_to_eol(void)
{
	write1(Ceol);   // Erase from cursor to end of line
}

//----- Erase from cursor to end of screen -----------------------
static void clear_to_eos(void)
{
	write1(Ceos);   // Erase from cursor to end of screen
}

//----- Start standout mode ------------------------------------
static void standout_start(void) // send "start reverse video" sequence
{
	write1(SOs);     // Start reverse video mode
}

//----- End standout mode --------------------------------------
static void standout_end(void) // send "end reverse video" sequence
{
	write1(SOn);     // End reverse video mode
}

//----- Flash the screen  --------------------------------------
static void flash(int h)
{
	standout_start();	// send "start reverse video" sequence
	redraw(TRUE);
	mysleep(h);
	standout_end();		// send "end reverse video" sequence
	redraw(TRUE);
}

static void Indicate_Error(void)
{
	if (!err_method) {
		write1(bell);   // send out a bell character
	} else {
		flash(10);
	}
}

//----- Screen[] Routines --------------------------------------
//----- Erase the Screen[] memory ------------------------------
static void screen_erase(void)
{
	memset(_G_pc_screen, ' ', _G_i_screensize);	// clear new screen
}

static int bufsum(char *buf, int count)
{
	int sum = 0;
	char *e = buf + count;

	while (buf < e)
		sum += (unsigned char) *buf++;
	return sum;
}

//----- Draw the status line at bottom of the screen -------------
static void show_status_line(void)
{
	int cnt = 0, cksum = 0;

	// either we already have an error or status message, or we
	// create one.
	if (!_G_i_have_status_msg) {
		cnt = format_edit_status();
		cksum = bufsum(_G_pc_status_buffer, cnt);
	}
	if (_G_i_have_status_msg || ((cnt > 0 && _G_i_last_status_cksum != cksum))) {
		_G_i_last_status_cksum = cksum;		// remember if we have seen this line
		place_cursor(_G_i_rows - 1, 0, FALSE);	// put cursor on status line
		write1(_G_pc_status_buffer);
		clear_to_eol();
		if (_G_i_have_status_msg) {
			if (((int)strlen(_G_pc_status_buffer) - (_G_i_have_status_msg - 1)) >
					(_G_i_columns - 1) ) {
				_G_i_have_status_msg = 0;
				Hit_Return();
			}
			_G_i_have_status_msg = 0;
		}
		place_cursor(_G_i_crow, _G_i_ccol, FALSE);	// put cursor back in correct place
	}
	fflush(stdout);
}

//----- format the status buffer, the bottom line of screen ------
// format status buffer, with STANDOUT mode
static void status_line_bold(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	strcpy(_G_pc_status_buffer, SOs);	// Terminal standout mode on
	vsprintf(_G_pc_status_buffer + sizeof(SOs)-1, format, args);
	strcat(_G_pc_status_buffer, SOn);	// Terminal standout mode off
	va_end(args);

	_G_i_have_status_msg = 1 + sizeof(SOs) + sizeof(SOn) - 2;
}

// format status buffer
static void status_line(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vsprintf(_G_pc_status_buffer, format, args);
	va_end(args);

	_G_i_have_status_msg = 1;
}

// copy s to buf, convert unprintable
static void print_literal(char *buf, const char *s)
{
	unsigned char c;
	char b[2];

	b[1] = '\0';
	buf[0] = '\0';
	if (!s[0])
		s = "(NULL)";
	for (; *s; s++) {
		int c_is_no_print;

		c = *s;
		c_is_no_print = (c & 0x80) && !Isprint(c);
		if (c_is_no_print) {
			strcat(buf, SOn);
			c = '.';
		}
		if (c < ' ' || c == 127) {
			strcat(buf, "^");
			if (c == 127)
				c = '?';
			else
				c += '@';
		}
		b[0] = c;
		strcat(buf, b);
		if (c_is_no_print)
			strcat(buf, SOs);
		if (*s == '\n')
			strcat(buf, "$");
		if (strlen(buf) > MAX_INPUT_LEN - 10) // paranoia
			break;
	}
}

static void not_implemented(const char *s)
{
	char buf[MAX_INPUT_LEN];

	print_literal(buf, s);
	status_line_bold("\'%s\' is not implemented", buf);
}

// show file status on status line
static int format_edit_status(void)
{
	static int tot;
	static const char cmd_mode_indicator[] = "-IR-";
	int cur, percent, ret, trunc_at;

	// _G_file_modified is now a counter rather than a flag.  this
	// helps reduce the amount of line counting we need to do.
	// (this will cause a mis-reporting of modified status
	// once every MAXINT editing operations.)

	// it would be nice to do a similar optimization here -- if
	// we haven't done a motion that could have changed which line
	// we're on, then we shouldn't have to do this count_lines()
	cur = count_lines(_GG_pc_text, _GG_pc_dot);

	// reduce counting -- the total lines can't have
	// changed if we haven't done any edits.
	if (_G_file_modified != _G_last_file_modified) {
		tot = cur + count_lines(_GG_pc_dot, _GG_pc_end - 1) - 1;
		_G_last_file_modified = _G_file_modified;
	}

	//    current line         percent
	//   -------------    ~~ ----------
	//    total lines            100
	if (tot > 0) {
		percent = (100 * cur) / tot;
	} else {
		cur = tot = 0;
		percent = 100;
	}

	trunc_at = _G_i_columns < STATUS_BUFFER_LEN-1 ?
		_G_i_columns : STATUS_BUFFER_LEN-1;

	ret = snprintf(_G_pc_status_buffer, trunc_at+1,
		"%c %s%s %d/%d %d%%",
		cmd_mode_indicator[_G_c_md_mode & 3],
		(_G_pc_current_filename != NULL ? _G_pc_current_filename : "No file"),
		(_G_file_modified ? " [Modified]" : ""),
		cur, tot, percent);

	if (ret >= 0 && ret < trunc_at)
		return ret;  /* it all fit */

	return trunc_at;  /* had to truncate */
}

//----- Force refresh of all Lines -----------------------------
static void redraw(int full_screen)
{
	place_cursor(0, 0, FALSE);	// put cursor in correct place
	clear_to_eos();		// tel terminal to erase display
	screen_erase();		// erase the internal screen buffer
	_G_i_last_status_cksum = 0;	// force status update
	refresh(full_screen);	// this will redraw the entire display
	show_status_line();
}

//----- Format a _GG_pc_text[] line into a buffer ---------------------
static char* format_line(char *src /*, int li*/)
{
	unsigned char c;
	int co;
	int ofs = _G_i_offset;
	char *dest = _GG_pc_scr_out_buf; // [MAX_SCR_COLS + MAX_TABSTOP * 2]

	c = '~'; // char in col 0 in non-existent lines is '~'
	co = 0;
	while (co < _G_i_columns + _G_i_tabstop) {
		// have we gone past the _GG_pc_end?
		if (src < _GG_pc_end) {
			c = *src++;
			if (c == '\n')
				break;
			if ((c & 0x80) && !Isprint(c)) {
				c = '.';
			}
			if (c < ' ' || c == 0x7f) {
				if (c == '\t') {
					c = ' ';
					//      co %    8     !=     7
					while ((co % _G_i_tabstop) != (_G_i_tabstop - 1)) {
						dest[co++] = c;
					}
				} else {
					dest[co++] = '^';
					if (c == 0x7f)
						c = '?';
					else
						c += '@'; // Ctrl-X -> 'X'
				}
			}
		}
		dest[co++] = c;
		// discard scrolled-off-to-the-left portion,
		// in tabstop-sized pieces
		if (ofs >= _G_i_tabstop && co >= _G_i_tabstop) {
			memmove(dest, dest + _G_i_tabstop, co);
			co -= _G_i_tabstop;
			ofs -= _G_i_tabstop;
		}
		if (src >= _GG_pc_end)
			break;
	}
	// check "short line, gigantic offset" case
	if (co < ofs)
		ofs = co;
	// discard last scrolled off part
	co -= ofs;
	dest += ofs;
	// fill the rest with spaces
	if (co < _G_i_columns)
		memset(&dest[co], ' ', _G_i_columns - co);
	return dest;
}

//----- Refresh the changed screen lines -----------------------
// Copy the source line from _GG_pc_text[] into the buffer and note
// if the current screenline is different from the new buffer.
// If they differ then that line needs redrawing on the terminal.
//
static void refresh(int full_screen)
{
	static int old_offset;

	int li, changed;
	char *tp, *sp;		// pointer into _GG_pc_text[] and _G_pc_screen[]

	sync_cursor(_GG_pc_dot, &_G_i_crow, &_G_i_ccol);	// where cursor will be (on "_GG_pc_dot")
	tp = _G_pc_screenbegin;	// index into _GG_pc_text[] of top line

	// compare _GG_pc_text[] to _G_pc_screen[] and mark _G_pc_screen[] lines that need updating
	for (li = 0; li < _G_i_rows - 1; li++) {
		int cs, ce;				// column start & end
		char *out_buf;
		// format current text line
		out_buf = format_line(tp /*, li*/);

		// skip to the end of the current _GG_pc_text[] line
		if (tp < _GG_pc_end) {
			char *t = memchr(tp, '\n', _GG_pc_end - tp);
			if (!t) t = _GG_pc_end - 1;
			tp = t + 1;
		}

		// see if there are any changes between vitual screen and out_buf
		changed = FALSE;	// assume no change
		cs = 0;
		ce = _G_i_columns - 1;
		sp = &_G_pc_screen[li * _G_i_columns];	// start of screen line
		if (full_screen) {
			// force re-draw of every single column from 0 - _G_i_columns-1
			goto re0;
		}
		// compare newly formatted buffer with virtual screen
		// look forward for first difference between buf and screen
		for (; cs <= ce; cs++) {
			if (out_buf[cs] != sp[cs]) {
				changed = TRUE;	// mark for redraw
				break;
			}
		}

		// look backward for last difference between out_buf and screen
		for (; ce >= cs; ce--) {
			if (out_buf[ce] != sp[ce]) {
				changed = TRUE;	// mark for redraw
				break;
			}
		}
		// now, cs is index of first diff, and ce is index of last diff

		// if horz offset has changed, force a redraw
		if (_G_i_offset != old_offset) {
re0:
			changed = TRUE;
		}

		// make a sanity check of columns indexes
		if (cs < 0) cs = 0;
		if (ce > _G_i_columns - 1) ce = _G_i_columns - 1;
		if (cs > ce) { cs = 0; ce = _G_i_columns - 1; }
		// is there a change between vitual screen and out_buf
		if (changed) {
			// copy changed part of buffer to virtual screen
			memcpy(sp+cs, out_buf+cs, ce-cs+1);
            
            // move cursor to column of first change
            place_cursor(li, cs, TRUE);

			// write line out to terminal
			fwrite(&sp[cs], ce - cs + 1, 1, stdout);
		}
	}

	place_cursor(_G_i_crow, _G_i_ccol, TRUE);

	old_offset = _G_i_offset;
}

//---------------------------------------------------------------------
//----- the Ascii Chart -----------------------------------------------
//
//  00 nul   01 soh   02 stx   03 etx   04 eot   05 enq   06 ack   07 bel
//  08 bs    09 ht    0a nl    0b vt    0c np    0d cr    0e so    0f si
//  10 dle   11 dc1   12 dc2   13 dc3   14 dc4   15 nak   16 syn   17 etb
//  18 can   19 em    1a sub   1b esc   1c fs    1d gs    1e rs    1f us
//  20 sp    21 !     22 "     23 #     24 $     25 %     26 &     27 '
//  28 (     29 )     2a *     2b +     2c ,     2d -     2e .     2f /
//  30 0     31 1     32 2     33 3     34 4     35 5     36 6     37 7
//  38 8     39 9     3a :     3b ;     3c <     3d =     3e >     3f ?
//  40 @     41 A     42 B     43 C     44 D     45 E     46 F     47 G
//  48 H     49 I     4a J     4b K     4c L     4d M     4e N     4f O
//  50 P     51 Q     52 R     53 S     54 T     55 U     56 V     57 W
//  58 X     59 Y     5a Z     5b [     5c \     5d ]     5e ^     5f _
//  60 `     61 a     62 b     63 c     64 d     65 e     66 f     67 g
//  68 h     69 i     6a j     6b k     6c l     6d m     6e n     6f o
//  70 p     71 q     72 r     73 s     74 t     75 u     76 v     77 w
//  78 x     79 y     7a z     7b {     7c |     7d }     7e ~     7f del
//---------------------------------------------------------------------

//----- Execute a Vi Command -----------------------------------
static void do_cmd(char c)
{
	const char *msg = msg; // for compiler
	char c1, *p, *q, *save_dot;
	char buf[12];
	int dir = dir; // for compiler
	int cnt, i, j;

//	c1 = c; // quiet the compiler
//	cnt = yf = 0; // quiet the compiler
//	msg = p = q = save_dot = buf; // quiet the compiler
	memset(buf, '\0', 12);

	show_status_line();

	/* if this is a cursor key, skip these checks */
	switch (c) {
		case VI_K_UP:
		case VI_K_DOWN:
		case VI_K_LEFT:
		case VI_K_RIGHT:
		case VI_K_HOME:
		case VI_K_END:
		case VI_K_PAGEUP:
		case VI_K_PAGEDOWN:
			goto key_cmd_mode;
	}

	if (_G_c_md_mode == 2) {
		//  flip-flop Insert/Replace mode
		if (c == VI_K_INSERT)
			goto dc_i;
		// we are 'R'eplacing the current *_GG_pc_dot with new char
		if (*_GG_pc_dot == '\n') {
			// don't Replace past E-o-l
			_G_c_md_mode = 1;	// convert to insert
		} else {
			if (1 <= c || Isprint(c)) {
				if (c != 27)
					_GG_pc_dot = yank_delete(_GG_pc_dot, _GG_pc_dot, 0, YANKDEL);	// delete char
				_GG_pc_dot = char_insert(_GG_pc_dot, c);	// insert new char
			}
			goto dc1;
		}
	}
	if (_G_c_md_mode == 1) {
		//  hitting "Insert" twice means "R" replace mode
		if (c == VI_K_INSERT) goto dc5;
		// insert the char c at "_GG_pc_dot"
		if (1 <= c || Isprint(c)) {
			_GG_pc_dot = char_insert(_GG_pc_dot, c);
		}
		goto dc1;
	}

key_cmd_mode:
	switch (c) {
	default:			// unrecognised command
		buf[0] = c;
		buf[1] = '\0';
		if (c < ' ') {
			buf[0] = '^';
			buf[1] = c + '@';
			buf[2] = '\0';
		}
		not_implemented(buf);
	case 0x00:			// nul- ignore
		break;
	case 2:			// ctrl-B  scroll up   full screen
	case VI_K_PAGEUP:	// Cursor Key Page Up
		dot_scroll(_G_i_rows - 2, -1);
		break;
	case 4:			// ctrl-D  scroll down half screen
		dot_scroll((_G_i_rows - 2) / 2, 1);
		break;
	case 5:			// ctrl-E  scroll down one line
		dot_scroll(1, 1);
		break;
	case 6:			// ctrl-F  scroll down full screen
	case VI_K_PAGEDOWN:	// Cursor Key Page Down
		dot_scroll(_G_i_rows - 2, 1);
		break;
	case 7:			// ctrl-G  show current status
		_G_i_last_status_cksum = 0;	// force status update
		break;
	case 'h':			// h- move left
	case VI_K_LEFT:	// cursor key Left
	case 8:		// ctrl-H- move left    (This may be ERASE char)
	case 0x7f:	// DEL- move left   (This may be ERASE char)
		if (_G_i_cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dot_left();
		break;
	case 10:			// Newline ^J
	case 'j':			// j- goto next line, same col
	case VI_K_DOWN:	// cursor key Down
		if (_G_i_cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dot_next();		// go to next B-o-l
		_GG_pc_dot = move_to_col(_GG_pc_dot, _G_i_ccol + _G_i_offset);	// try stay in same col
		break;
	case 12:			// ctrl-L  force redraw whole screen
	case 18:			// ctrl-R  force redraw
		place_cursor(0, 0, FALSE);	// put cursor in correct place
		clear_to_eos();	// tel terminal to erase display
		mysleep(10);
		screen_erase();	// erase the internal screen buffer
		_G_i_last_status_cksum = 0;	// force status update
		refresh(TRUE);	// this will redraw the entire display
		break;
	case 13:			// Carriage Return ^M
	case '+':			// +- goto next line
		if (_G_i_cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dot_next();
		dot_skip_over_ws();
		break;
	case 21:			// ctrl-U  scroll up   half screen
		dot_scroll((_G_i_rows - 2) / 2, -1);
		break;
	case 25:			// ctrl-Y  scroll up one line
		dot_scroll(1, -1);
		break;
	case 27:			// esc
		if (_G_c_md_mode == 0)
			indicate_error(c);
		_G_c_md_mode = 0;	// stop insrting
		_G_i_last_status_cksum = 0;	// force status update
		break;
	case ' ':			// move right
	case 'l':			// move right
	case VI_K_RIGHT:	// Cursor Key Right
		if (_G_i_cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dot_right();
		break;

	case '$':			// $- goto _GG_pc_end of line
	case VI_K_END:		// Cursor Key End
		if (_G_i_cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		_GG_pc_dot = end_line(_GG_pc_dot);
		break;
	case '%':			// %- find matching char of pair () [] {}
		for (q = _GG_pc_dot; q < _GG_pc_end && *q != '\n'; q++) {
			if (strchr("()[]{}", *q) != NULL) {
				// we found half of a pair
				p = find_pair(q, *q);
				if (p == NULL) {
					indicate_error(c);
				} else {
					_GG_pc_dot = p;
				}
				break;
			}
		}
		if (*q == '\n')
			indicate_error(c);
		break;
	case 'f':			// f- forward to a user specified char
		_G_c_last_forward_char = get_one_char();	// get the search char
		//
		// dont separate these two commands. 'f' depends on ';'
		//
		//**** fall through to ... ';'
	case ';':			// ;- look at rest of line for last forward char
		if (_G_i_cmdcnt-- > 1) {
			do_cmd(';');
		}				// repeat cnt
		if (_G_c_last_forward_char == 0)
			break;
		q = _GG_pc_dot + 1;
		while (q < _GG_pc_end - 1 && *q != '\n' && *q != _G_c_last_forward_char) {
			q++;
		}
		if (*q == _G_c_last_forward_char)
			_GG_pc_dot = q;
		break;
	case ',':           // repeat latest 'f' in opposite direction
		if (_G_i_cmdcnt-- > 1) {
			do_cmd(',');
		}                               // repeat cnt
		if (_G_c_last_forward_char == 0)
			break;
		q = _GG_pc_dot - 1;
		while (q >= _GG_pc_text && *q != '\n' && *q != _G_c_last_forward_char) {
			q--;
		}
		if (q >= _GG_pc_text && *q == _G_c_last_forward_char)
			_GG_pc_dot = q;
		break;

	case '-':			// -- goto prev line
		if (_G_i_cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dot_prev();
		dot_skip_over_ws();
		break;


	case '0':			// 0- goto begining of line
	case '1':			// 1-
	case '2':			// 2-
	case '3':			// 3-
	case '4':			// 4-
	case '5':			// 5-
	case '6':			// 6-
	case '7':			// 7-
	case '8':			// 8-
	case '9':			// 9-
		if (c == '0' && _G_i_cmdcnt < 1) {
			dot_begin();	// this was a standalone zero
		} else {
			_G_i_cmdcnt = _G_i_cmdcnt * 10 + (c - '0');	// this 0 is part of a number
		}
		break;
	case ':':			// :- the colon mode commands
		p = get_input_line(":");	// get input line- use "status line"

		if (*p == ':')
			p++;				// move past the ':'
		cnt = strlen(p);
		if (cnt <= 0)
			break;
		if (strncasecmp(p, "quit", cnt) == 0
		 || strncasecmp(p, "q!", cnt) == 0   // delete lines
		) {
			if (_G_file_modified && p[1] != '!') {
				status_line_bold("No write since last change (:quit! overrides)");
			} else {
				_G_editing = 0;
			}
		} else if (strncasecmp(p, "write", cnt) == 0
		        || strncasecmp(p, "wq", cnt) == 0
		        || strncasecmp(p, "wn", cnt) == 0
		        || strncasecmp(p, "x", cnt) == 0
		) {
			cnt = file_write(_G_pc_current_filename, _GG_pc_text, _GG_pc_end - 1);
			if (cnt < 0) {
				if (cnt == -1)
					status_line_bold("Write error: %s", strerror(errno));
			} else {
				_G_file_modified = 0;
				_G_last_file_modified = -1;
				status_line("\"%s\" %dL, %dC", _G_pc_current_filename, count_lines(_GG_pc_text, _GG_pc_end - 1), cnt);
				if (p[0] == 'x' || p[1] == 'q' || p[1] == 'n'
				 || p[0] == 'X' || p[1] == 'Q' || p[1] == 'N'
				) {
					_G_editing = 0;
				}
			}
		} else if (strncasecmp(p, "file", cnt) == 0) {
			_G_i_last_status_cksum = 0;	// force status update
		} else if (sscanf(p, "%d", &j) > 0) {
			_GG_pc_dot = find_line(j);		// go to line # j
			dot_skip_over_ws();
		} else {		// unrecognised cmd
			not_implemented(p);
		}
		break;
	case '<':			// <- Left  shift something
	case '>':			// >- Right shift something
		cnt = count_lines(_GG_pc_text, _GG_pc_dot);	// remember what line we are on
		c1 = get_one_char();	// get the type of thing to delete
		find_range(&p, &q, c1);
		yank_delete(p, q, 1, YANKONLY);	// save copy before change
		p = begin_line(p);
		q = end_line(q);
		i = count_lines(p, q);	// # of lines we are shifting
		for ( ; i > 0; i--, p = next_line(p)) {
			if (c == '<') {
				// shift left- remove tab or 8 spaces
				if (*p == '\t') {
					// shrink buffer 1 char
					text_hole_delete(p, p);
				} else if (*p == ' ') {
					// we should be calculating columns, not just SPACE
					for (j = 0; *p == ' ' && j < _G_i_tabstop; j++) {
						text_hole_delete(p, p);
					}
				}
			} else if (c == '>') {
				// shift right -- add tab or 8 spaces
				char_insert(p, '\t');
			}
		}
		_GG_pc_dot = find_line(cnt);	// what line were we on
		dot_skip_over_ws();
		break;
	case 'A':			// A- append at e-o-l
		dot_end();		// go to e-o-l
		//**** fall through to ... 'a'
	case 'a':			// a- append after current char
		if (*_GG_pc_dot != '\n')
			_GG_pc_dot++;
		goto dc_i;
		break;
	case 'B':			// B- back a blank-delimited Word
	case 'E':			// E- end of a blank-delimited word
	case 'W':			// W- forward a blank-delimited word
		if (_G_i_cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dir = FORWARD;
		if (c == 'B')
			dir = BACK;
		if (c == 'W' || isspace(_GG_pc_dot[dir])) {
			_GG_pc_dot = skip_thing(_GG_pc_dot, 1, dir, S_TO_WS);
			_GG_pc_dot = skip_thing(_GG_pc_dot, 2, dir, S_OVER_WS);
		}
		if (c != 'W')
			_GG_pc_dot = skip_thing(_GG_pc_dot, 1, dir, S_BEFORE_WS);
		break;
	case 'C':			// C- Change to e-o-l
	case 'D':			// D- delete to e-o-l
		save_dot = _GG_pc_dot;
		_GG_pc_dot = dollar_line(_GG_pc_dot);	// move to before NL
		// copy text into a register and delete
		_GG_pc_dot = yank_delete(save_dot, _GG_pc_dot, 0, YANKDEL);	// delete to e-o-l
		if (c == 'C')
			goto dc_i;	// start inserting

		break;
	case 'g':                       // 'gg' goto a line number (from vim)
					// (default to first line in file)
		c1 = get_one_char();
		if (c1 != 'g') {
			buf[0] = 'g';
			buf[1] = c1;
			buf[2] = '\0';
			not_implemented(buf);
			break;
		}
		if (_G_i_cmdcnt == 0)
			_G_i_cmdcnt = 1;
		/* fall through */
	case 'G':		// G- goto to a line number (default= E-O-F)
		_GG_pc_dot = _GG_pc_end - 1;				// assume E-O-F
		if (_G_i_cmdcnt > 0) {
			_GG_pc_dot = find_line(_G_i_cmdcnt);	// what line is #_G_i_cmdcnt
		}
		dot_skip_over_ws();
		break;
	case 'H':			// H- goto top line on screen
		_GG_pc_dot = _G_pc_screenbegin;
		if (_G_i_cmdcnt > (_G_i_rows - 1)) {
			_G_i_cmdcnt = (_G_i_rows - 1);
		}
		if (_G_i_cmdcnt-- > 1) {
			do_cmd('+');
		}				// repeat cnt
		dot_skip_over_ws();
		break;
	case 'I':			// I- insert before first non-blank
		dot_begin();	// 0
		dot_skip_over_ws();
		//**** fall through to ... 'i'
	case 'i':			// i- insert before current char
	case VI_K_INSERT:	// Cursor Key Insert
dc_i:
		_G_c_md_mode = 1;	// start insrting
		break;
	case 'J':			// J- join current and next lines together
		if (_G_i_cmdcnt-- > 2) {
			do_cmd(c);
		}				// repeat cnt
		dot_end();		// move to NL
		if (_GG_pc_dot < _GG_pc_end - 1) {	// make sure not last char in _GG_pc_text[]
			*_GG_pc_dot++ = ' ';	// replace NL with space
			_G_file_modified++;
			while (isblank(*_GG_pc_dot)) {	// delete leading WS
				dot_delete();
			}
		}
		break;
	case 'L':			// L- goto bottom line on screen
		_GG_pc_dot = end_screen();
		if (_G_i_cmdcnt > (_G_i_rows - 1)) {
			_G_i_cmdcnt = (_G_i_rows - 1);
		}
		if (_G_i_cmdcnt-- > 1) {
			do_cmd('-');
		}				// repeat cnt
		dot_begin();
		dot_skip_over_ws();
		break;
	case 'M':			// M- goto middle line on screen
		_GG_pc_dot = _G_pc_screenbegin;
		for (cnt = 0; cnt < (_G_i_rows-1) / 2; cnt++)
			_GG_pc_dot = next_line(_GG_pc_dot);
		break;
	case 'O':			// O- open a empty line above
		//    0i\n ESC -i
		p = begin_line(_GG_pc_dot);
		if (p[-1] == '\n') {
			dot_prev();
	case 'o':			// o- open a empty line below; Yes, I know it is in the middle of the "if (..."
			dot_end();
			_GG_pc_dot = char_insert(_GG_pc_dot, '\n');
		} else {
			dot_begin();	// 0
			_GG_pc_dot = char_insert(_GG_pc_dot, '\n');	// i\n ESC
			dot_prev();	// -
		}
		goto dc_i;
		break;
	case 'R':			// R- continuous Replace char
dc5:
		_G_c_md_mode = 2;
		break;
	case VI_K_DELETE:
		c = 'x';
		// fall through
	case 'X':			// X- delete char before dot
	case 'x':			// x- delete the current char
	case 's':			// s- substitute the current char
		if (_G_i_cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dir = 0;
		if (c == 'X')
			dir = -1;
		if (_GG_pc_dot[dir] != '\n') {
			if (c == 'X')
				_GG_pc_dot--;	// delete prev char
			_GG_pc_dot = yank_delete(_GG_pc_dot, _GG_pc_dot, 0, YANKDEL);	// delete char
		}
		if (c == 's')
			goto dc_i;	// start insrting
		break;
	case 'Z':			// Z- if modified, {write}; exit
		// ZZ means to save file (if necessary), then exit
		c1 = get_one_char();
		if (c1 != 'Z') {
			indicate_error(c);
			break;
		}
		if (_G_file_modified) {
			cnt = file_write(_G_pc_current_filename, _GG_pc_text, _GG_pc_end - 1);
			if (cnt < 0) {
				if (cnt == -1)
					status_line_bold("Write error: %s", strerror(errno));
			} else if (cnt == (_GG_pc_end - 1 - _GG_pc_text + 1)) {
				_G_editing = 0;
			}
		} else {
			_G_editing = 0;
		}
		break;
	case '^':			// ^- move to first non-blank on line
		dot_begin();
		dot_skip_over_ws();
		break;
	case 'b':			// b- back a word
	case 'e':			// e- end of word
		if (_G_i_cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dir = FORWARD;
		if (c == 'b')
			dir = BACK;
		if ((_GG_pc_dot + dir) < _GG_pc_text || (_GG_pc_dot + dir) > _GG_pc_end - 1)
			break;
		_GG_pc_dot += dir;
		if (isspace(*_GG_pc_dot)) {
			_GG_pc_dot = skip_thing(_GG_pc_dot, (c == 'e') ? 2 : 1, dir, S_OVER_WS);
		}
		if (isalnum(*_GG_pc_dot) || *_GG_pc_dot == '_') {
			_GG_pc_dot = skip_thing(_GG_pc_dot, 1, dir, S_END_ALNUM);
		} else if (ispunct(*_GG_pc_dot)) {
			_GG_pc_dot = skip_thing(_GG_pc_dot, 1, dir, S_END_PUNCT);
		}
		break;
	case 'c':			// c- change something
	case 'd':			// d- delete something
		{
		int yf, ml, whole = 0;
		yf = YANKDEL;	// assume either "c" or "d"

		c1 = 'y';
		if (c != 'Y')
			c1 = get_one_char();	// get the type of thing to delete
		// determine range, and whether it spans lines
		ml = find_range(&p, &q, c1);
		if (c1 == 27) {	// ESC- user changed mind and wants out
			c = c1 = 27;	// Escape- do nothing
		} else if (strchr("wW", c1)) {
			if (c == 'c') {
				// don't include trailing WS as part of word
				while (isblank(*q)) {
					if (q <= _GG_pc_text || q[-1] == '\n')
						break;
					q--;
				}
			}
			_GG_pc_dot = yank_delete(p, q, ml, yf);	// delete word
		} else if (strchr("^0bBeEft%$ lh\b\177", c1)) {
			// partial line copy _GG_pc_text into a register and delete
			_GG_pc_dot = yank_delete(p, q, ml, yf);	// delete word
		} else if (strchr("cdykjHL+-{}\r\n", c1)) {
			// whole line copy text into a register and delete
			_GG_pc_dot = yank_delete(p, q, ml, yf);	// delete lines
			whole = 1;
		} else {
			// could not recognize object
			c = c1 = 27;	// error-
			ml = 0;
			indicate_error(c);
		}
		if (ml && whole) {
			if (c == 'c') {
				_GG_pc_dot = char_insert(_GG_pc_dot, '\n');
				// on the last line of file don't move to prev line
				if (whole && _GG_pc_dot != (_GG_pc_end-1)) {
					dot_prev();
				}
			} else if (c == 'd') {
				dot_begin();
				dot_skip_over_ws();
			}
		}
		if (c1 != 27) {
			// if CHANGING, not deleting, start inserting after the delete
			if (c == 'c') {
				strcpy(buf, "Change");
				goto dc_i;	// start inserting
			}
			if (c == 'd') {
				strcpy(buf, "Delete");
			}
		}
		}
		break;
	case 'k':			// k- goto prev line, same col
	case VI_K_UP:		// cursor key Up
		if (_G_i_cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dot_prev();
		_GG_pc_dot = move_to_col(_GG_pc_dot, _G_i_ccol + _G_i_offset);	// try stay in same col
		break;
	case 'r':			// r- replace the current char with user input
		c1 = get_one_char();	// get the replacement char
		if (*_GG_pc_dot != '\n') {
			*_GG_pc_dot = c1;
			_G_file_modified++;	// has the file been modified
		}
		break;
	case 't':			// t- move to char prior to next x
		_G_c_last_forward_char = get_one_char();
		do_cmd(';');
		if (*_GG_pc_dot == _G_c_last_forward_char)
			dot_left();
		_G_c_last_forward_char= 0;
		break;
	case 'w':			// w- forward a word
		if (_G_i_cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		if (isalnum(*_GG_pc_dot) || *_GG_pc_dot == '_') {	// we are on ALNUM
			_GG_pc_dot = skip_thing(_GG_pc_dot, 1, FORWARD, S_END_ALNUM);
		} else if (ispunct(*_GG_pc_dot)) {	// we are on PUNCT
			_GG_pc_dot = skip_thing(_GG_pc_dot, 1, FORWARD, S_END_PUNCT);
		}
		if (_GG_pc_dot < _GG_pc_end - 1)
			_GG_pc_dot++;		// move over word
		if (isspace(*_GG_pc_dot)) {
			_GG_pc_dot = skip_thing(_GG_pc_dot, 2, FORWARD, S_OVER_WS);
		}
		break;
	case 'z':			// z-
		c1 = get_one_char();	// get the replacement char
		cnt = 0;
		if (c1 == '.')
			cnt = (_G_i_rows - 2) / 2;	// put _GG_pc_dot at center
		if (c1 == '-')
			cnt = _G_i_rows - 2;	// put _GG_pc_dot at bottom
		_G_pc_screenbegin = begin_line(_GG_pc_dot);	// start _GG_pc_dot at top
		dot_scroll(cnt, -1);
		break;
	case '|':			// |- move to column "_G_i_cmdcnt"
		_GG_pc_dot = move_to_col(_GG_pc_dot, _G_i_cmdcnt - 1);	// try to move to column
		break;
	case '~':			// ~- flip the case of letters   a-z -> A-Z
		if (_G_i_cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		if (islower(*_GG_pc_dot)) {
			*_GG_pc_dot = toupper(*_GG_pc_dot);
			_G_file_modified++;	// has the file been modified
		} else if (isupper(*_GG_pc_dot)) {
			*_GG_pc_dot = tolower(*_GG_pc_dot);
			_G_file_modified++;	// has the file been modified
		}
		dot_right();
		break;
		//----- The Cursor and Function Keys -----------------------------
	case VI_K_HOME:	// Cursor Key Home
		dot_begin();
		break;
		// The Fn keys could point to do_macro which could translate them
	case VI_K_FUN1:	// Function Key F1
	case VI_K_FUN2:	// Function Key F2
	case VI_K_FUN3:	// Function Key F3
	case VI_K_FUN4:	// Function Key F4
	case VI_K_FUN5:	// Function Key F5
	case VI_K_FUN6:	// Function Key F6
	case VI_K_FUN7:	// Function Key F7
	case VI_K_FUN8:	// Function Key F8
	case VI_K_FUN9:	// Function Key F9
	case VI_K_FUN10:	// Function Key F10
	case VI_K_FUN11:	// Function Key F11
	case VI_K_FUN12:	// Function Key F12
		break;
	}

dc1:
	// if _GG_pc_text[] just became empty, add back an empty line
	if (_GG_pc_end == _GG_pc_text) {
		char_insert(_GG_pc_text, '\n');	// start empty buf with dummy line
		_GG_pc_dot = _GG_pc_text;
	}
	// it is OK for _GG_pc_dot to exactly equal to end, otherwise check _GG_pc_dot validity
	if (_GG_pc_dot != _GG_pc_end) {
		_GG_pc_dot = bound_dot(_GG_pc_dot);	// make sure "_GG_pc_dot" is valid
	}


	if (!isdigit(c))
		_G_i_cmdcnt = 0;		// cmd was not a number, reset _G_i_cmdcnt
	cnt = _GG_pc_dot - begin_line(_GG_pc_dot);
	// Try to stay off of the Newline
	if (*_GG_pc_dot == '\n' && cnt > 0 && _G_c_md_mode == 0)
		_GG_pc_dot--;
}
