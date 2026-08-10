/* NetHack 5.0  winswift.h */
/* Interface between NetHack's "swift" windowport and a Swift/Objective-C
 * front end.
 *
 * This header deliberately includes NO NetHack headers.  Everything here is
 * plain C using standard types, so it can be imported by a Swift package
 * (via a module map or an umbrella header) without dragging in hack.h,
 * config.h, or NetHack's boolean/TRUE/FALSE macros, which collide with
 * Foundation.
 *
 * All NetHack-specific types are translated on the C side in winswift.c:
 *   winid    -> int
 *   boolean  -> int (0 / 1)
 *   coordxy  -> int
 *   anything -> an integer item index (see add_menu / select_menu)
 *   glyph_info -> nhswift_glyph (below)
 */

#ifndef WINSWIFT_H
#define WINSWIFT_H

#include <stddef.h>
#include <stdint.h>  /* uintptr_t */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Constants mirrored from NetHack headers.                            */
/* winswift.c contains compile-time checks that these still match.     */
/* ------------------------------------------------------------------ */

enum nhswift_wintype {
	NHSWIFT_WIN_MESSAGE = 1,
	NHSWIFT_WIN_STATUS  = 2,
	NHSWIFT_WIN_MAP     = 3,
	NHSWIFT_WIN_MENU    = 4,
	NHSWIFT_WIN_TEXT    = 5
};

enum nhswift_pick {
	NHSWIFT_PICK_NONE = 0,
	NHSWIFT_PICK_ONE  = 1,
	NHSWIFT_PICK_ANY  = 2
};

/* Returned by select_menu when the user cancelled (ESC). */
#define NHSWIFT_MENU_CANCELLED (-1)

/* Size of the buffer handed to getlin() and getmsghistory(). */
#define NHSWIFT_BUFSZ 256

/* ------------------------------------------------------------------ */
/* Menu selection result.                                              */
/*                                                                     */
/* Mirrors the layout of NetHack's menu_item (anything item + long     */
/* count) so winswift.c can convert between the two with a simple      */
/* field-by-field copy.  The `identifier` field carries the anything   */
/* union reinterpreted as a uintptr_t.                                 */
/* ------------------------------------------------------------------ */

typedef struct {
	uintptr_t identifier;  /* anything value, passed back unchanged */
	long      count;       /* selection multiplier; -1 means "all" */
	unsigned  itemflags;   /* item flags (mirrors menu_item.itemflags) */
} nhswift_menu_item;

/* ------------------------------------------------------------------ */
/* Glyph description.                                                  */
/* ------------------------------------------------------------------ */

typedef struct nhswift_glyph {
	int glyph;           /* raw glyph number */
	int ttychar;         /* the ASCII character a tty port would draw */
	int color;           /* CLR_* index, 0..15 */
	int symidx;          /* index into the symbol set */
	unsigned glyphflags; /* MG_* flags: pet, detected, ridden, ... */
	int tileidx;         /* tile number, for tile-based rendering */
} nhswift_glyph;

/* ------------------------------------------------------------------ */
/* Status fields.                                                      */
/* ------------------------------------------------------------------ */

/* status_update() is called once per changed field, then once with
 * fldidx == NHSWIFT_BL_FLUSH meaning "you now have a consistent set".
 * Update your model incrementally and publish on FLUSH.
 *
 * For most fields, `text` holds the preformatted value and `value` is 0.
 * For NHSWIFT_BL_CONDITION, `condbits` holds the bitmask of active
 * conditions and `text` is NULL.
 */
#define NHSWIFT_BL_FLUSH           (-1)
#define NHSWIFT_BL_RESET           (-2)
#define NHSWIFT_BL_CHARACTERISTICS (-3)
#define NHSWIFT_BL_CONDITION 22
#define NHSWIFT_MAXBLSTATS   27

/* ------------------------------------------------------------------ */
/* The callback table.                                                 */
/* ------------------------------------------------------------------ */

/* NAMING: these members are deliberately camelCase.  include/winprocs.h
 * turns nearly every windowport name into an object-like macro --
 *     #define putstr (*windowprocs.win_putstr)
 * -- so a member named `putstr` breaks both this declaration and every
 * `cb.putstr` use site in winswift.c, no matter what order things are
 * included in.  NetHack's house style is strictly snake_case, so
 * camelCase members cannot collide.  Do not "tidy" these back to
 * snake_case.
 *
 * Every member may be NULL; winswift.c substitutes a safe default
 * (usually a no-op, or an ESC / "no" answer for input routines).
 *
 * IMPORTANT: all of these are called on NetHack's game thread, never on
 * the main thread.  Marshal to the main actor yourself.  The input
 * routines (nhgetch, yn_function, getlin, select_menu, nh_poskey,
 * get_ext_cmd) are expected to BLOCK until the user has answered.
 */
typedef struct nhswift_callbacks {
	/* --- lifecycle --- */
	void (*initWindows)(int *argcp, char **argv);
	void (*exitWindows)(const char *lastgasp);
	void (*suspendWindows)(const char *str);
	void (*resumeWindows)(void);

	/* --- character creation --- */
	/* Return 1 to let NetHack run its own built-in selection dialog,
	 * 0 if you have populated the role/race/gender/align yourself. */
	int (*playerSelection)(void);
	/* Fill NetHack's player name.  Write into buf (NUL-terminated). */
	void (*askName)(char *buf, int bufsize);

	/* --- windows --- */
	int  (*createWindow)(int type);   /* return your own window id, or 0 */
	void (*clearWindow)(int window);
	void (*displayWindow)(int window, int blocking);
	void (*destroyWindow)(int window);
	void (*moveCursor)(int window, int x, int y);
	void (*putString)(int window, int attr, const char *str);
	void (*displayFile)(const char *name, int complain);

	/* --- map --- */
	void (*printGlyph)(int window, int x, int y,
						const nhswift_glyph *gi,
						const nhswift_glyph *bkgi);
	void (*clipAround)(int x, int y);

	/* --- menus --- */
	void (*startMenu)(int window, unsigned long mbehavior);
	/* `identifier` is the anything value for this item, passed back verbatim
	 * from select_menu().  Zero means the item is a non-selectable heading. */
	void (*addMenu)(int window, const nhswift_glyph *gi,
					 int ch, int gch, int attr, int clr,
					 const char *str, unsigned int itemflags,
					 uintptr_t identifier);
	void (*endMenu)(int window, const char *prompt);
	/* Allocate and return (via *out_items) an array of chosen items;
	 * return how many were selected, 0 for none, or NHSWIFT_MENU_CANCELLED
	 * if the user backed out.  The caller owns the array and frees it with
	 * free().  *out_items may be left NULL when the return value is <= 0. */
	int  (*selectMenu)(int window, int how,
						nhswift_menu_item **out_items);
	/* Feedback from the --More-- prompt; return 0 if you don't handle it. */
	int  (*messageMenu)(int let, int how, const char *mesg);

	/* --- input --- */
	int  (*getChar)(void);
	int  (*posKey)(int *x, int *y, int *mod);
	int  (*ynFunction)(const char *query, const char *resp, int def);
	void (*getLine)(const char *query, char *buf, int bufsize);
	int  (*getExtCmd)(void);
	int  (*prevMessage)(void);
	void (*numberPad)(int state);

	/* --- synchronization / misc --- */
	void (*getEvent)(void);
	void (*markSynch)(void);
	void (*waitSynch)(void);
	void (*delayOutput)(void);
	void (*bell)(void);
	void (*rawPrint)(const char *str);
	void (*rawPrintBold)(const char *str);
	void (*preferenceUpdate)(const char *pref);
	void (*updateInventory)(int arg);
	void (*updatePositionBar)(const char *posbar);

	/* --- message history (save/restore) --- */
	/* Return 1 and fill buf with the next message to save, oldest first;
	 * return 0 when there are no more. */
	int  (*getMsgHistory)(int init, char *buf, int bufsize);
	void (*putMsgHistory)(const char *msg, int restoring);

	/* --- status --- */
	void (*statusInit)(void);
	void (*statusEnableField)(int fieldidx, const char *nm,
							   const char *fmt, int enable);
	void (*statusUpdate)(int fldidx, const char *text, long condbits,
						  int chg, int percent, int color,
						  const unsigned long *colormasks);

	/* --- colors (only used when NetHack is built with CHANGE_COLOR) --- */
	void (*changeColor)(int color, long rgb, int reverse);
	const char *(*getColorString)(void);
} nhswift_callbacks;

/* ------------------------------------------------------------------ */
/* Entry points.                                                       */
/* ------------------------------------------------------------------ */

/* Set the paths for resources and playground */
void nhswift_set_paths(const char *hackdir, const char *playground);

/* Install the callback table.  MUST be called before choose_windows()
 * runs, i.e. before you start the game.  The table is copied, so the
 * caller does not need to keep it alive.  Passing NULL clears it. */
void nhswift_set_callbacks(const nhswift_callbacks *cb);

#ifdef __cplusplus
}
#endif

#endif /* WINSWIFT_H */
