/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  note.h: notes system header file                                     */
/*                                                                       */
/*************************************************************************/

#pragma once

struct CHAR_DATA;
typedef struct note_data NOTE_DATA;

#define NOTE_FILE "notes.txt" /* For 'notes'                  */

void do_note(CHAR_DATA *ch, const char *argument);
void note_initialise();
int note_count(CHAR_DATA *ch);
int is_note_to(CHAR_DATA *ch, NOTE_DATA *note);
void note_announce(CHAR_DATA *chsender, NOTE_DATA *note);
