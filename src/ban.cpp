/***************************************************************************
 *	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
 *	ROM has been brought to you by the ROM consortium		   *
 *	    Russ Taylor (rtaylor@pacinfo.com)				   *
 *	    Gabrielle Taylor (gtaylor@pacinfo.com)			   *
 *	    Brian Moore (rom@rom.efn.org)				   *
 *	By using this code, you have agreed to follow the terms of the	   *
 *	ROM license, in the file Rom24/doc/rom.license			   *
 ***************************************************************************/

/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*  Additional: ROM2.4 copyright as stated at top of file: ban.c         */
/*                                                                       */
/*  ban.c: very useful utilities for keeping certain people out!         */
/*                                                                       */
/*************************************************************************/

#include "buffer.h"
#include "merc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

char *print_flags(const int value);

BAN_DATA *ban_list;

BAN_DATA *new_ban() {
    BAN_DATA *res;
    res = (BAN_DATA *)(malloc(sizeof(BAN_DATA)));
    res->level = 0;
    res->name = nullptr;
    res->next = nullptr;
    res->ban_flags = 0;
    return res;
}

void free_ban(BAN_DATA *foo) {
    free_string(foo->name);
    free(foo);
}

void save_bans() {
    BAN_DATA *pban;
    FILE *fp;
    bool found = false;

    fclose(fpReserve);
    if ((fp = fopen(BAN_FILE, "w")) == nullptr) {
        perror(BAN_FILE);
    }

    for (pban = ban_list; pban != nullptr; pban = pban->next) {
        if (IS_SET(pban->ban_flags, BAN_PERMANENT)) {
            found = true;
            fprintf(fp, "%s %d %s\n", pban->name, pban->level, print_flags(pban->ban_flags));
        }
    }

    fclose(fp);
    fpReserve = fopen(NULL_FILE, "r");
    if (!found)
        unlink(BAN_FILE);
}

void load_bans() {
    FILE *fp;
    BAN_DATA *ban_last;

    if ((fp = fopen(BAN_FILE, "r")) == nullptr)
        return;

    ban_last = nullptr;
    for (;;) {
        BAN_DATA *pban;
        if (feof(fp)) {
            fclose(fp);
            return;
        }

        pban = new_ban();

        pban->name = str_dup(fread_word(fp));
        pban->level = fread_number(fp);
        pban->ban_flags = fread_flag(fp);
        pban->next = nullptr;
        fread_to_eol(fp);

        if (ban_list == nullptr)
            ban_list = pban;
        else
            ban_last->next = pban;
        ban_last = pban;
    }
}

bool check_ban(char *site, int type) {
    BAN_DATA *pban;
    char host[MAX_STRING_LENGTH];

    strcpy(host, capitalize(site));
    host[0] = LOWER(host[0]);

    for (pban = ban_list; pban != nullptr; pban = pban->next) {
        if (!IS_SET(pban->ban_flags, type))
            continue;

        if (IS_SET(pban->ban_flags, BAN_PREFIX) && IS_SET(pban->ban_flags, BAN_SUFFIX)
            && strstr(pban->name, host) != nullptr)
            return true;

        if (IS_SET(pban->ban_flags, BAN_PREFIX) && !str_suffix(pban->name, host))
            return true;

        if (IS_SET(pban->ban_flags, BAN_SUFFIX) && !str_prefix(pban->name, host))
            return true;

        if (!IS_SET(pban->ban_flags, BAN_SUFFIX) && !IS_SET(pban->ban_flags, BAN_PREFIX) && !str_cmp(pban->name, host))
            return true;
    }

    return false;
}

void ban_site(CHAR_DATA *ch, char *argument, bool fPerm) {
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *name;
    BUFFER *buffer;
    BAN_DATA *pban, *prev;
    bool prefix = false, suffix = false;
    int type;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0') {
        if (ban_list == nullptr) {
            send_to_char("No sites banned at this time.\n\r", ch);
            return;
        }
        buffer = buffer_create();

        buffer_addline(buffer, "Banned sites       Level  Type     Status\n\r");
        for (pban = ban_list; pban != nullptr; pban = pban->next) {
            snprintf(buf, sizeof(buf), "%s%s%s", IS_SET(pban->ban_flags, BAN_PREFIX) ? "*" : "", pban->name,
                     IS_SET(pban->ban_flags, BAN_SUFFIX) ? "*" : "");
            buffer_addline_fmt(
                buffer, "%-17s    %-3d  %-7s  %s\n\r", buf, pban->level,
                IS_SET(pban->ban_flags, BAN_NEWBIES)
                    ? "newbies"
                    : IS_SET(pban->ban_flags, BAN_PERMIT) ? "permit" : IS_SET(pban->ban_flags, BAN_ALL) ? "all" : "",
                IS_SET(pban->ban_flags, BAN_PERMANENT) ? "perm" : "temp");
        }
        page_to_char(buffer_string(buffer), ch);
        buffer_destroy(buffer);
        return;
    }

    /* find out what type of ban */
    if (arg2[0] == '\0' || !str_prefix(arg2, "all"))
        type = BAN_ALL;
    else if (!str_prefix(arg2, "newbies"))
        type = BAN_NEWBIES;
    else if (!str_prefix(arg2, "permit"))
        type = BAN_PERMIT;
    else {
        send_to_char("Acceptable ban types are 'all', 'newbies', and 'permit'.\n\r", ch);
        return;
    }

    name = arg1;

    if (name[0] == '*') {
        prefix = true;
        name++;
    }

    if (name[strlen(name) - 1] == '*') {
        suffix = true;
        name[strlen(name) - 1] = '\0';
    }

    if (strlen(name) == 0) {
        send_to_char("Ban which site?\n\r", ch);
        return;
    }

    prev = nullptr;
    for (pban = ban_list; pban != nullptr; prev = pban, pban = pban->next) {
        if (!str_cmp(name, pban->name)) {
            if (pban->level > get_trust(ch)) {
                send_to_char("That ban was set by a higher power.\n\r", ch);
                return;
            } else {
                if (prev == nullptr)
                    ban_list = pban->next;
                else
                    prev->next = pban->next;
                free_ban(pban);
            }
        }
    }

    pban = new_ban();
    pban->name = str_dup(name);
    pban->level = get_trust(ch);

    /* set ban type */
    pban->ban_flags = type;

    if (prefix)
        SET_BIT(pban->ban_flags, BAN_PREFIX);
    if (suffix)
        SET_BIT(pban->ban_flags, BAN_SUFFIX);
    if (fPerm)
        SET_BIT(pban->ban_flags, BAN_PERMANENT);

    pban->next = ban_list;
    ban_list = pban;
    save_bans();
    snprintf(buf, sizeof(buf), "The host(s) matching '%s%s%s' have been banned.\n\r",
             IS_SET(pban->ban_flags, BAN_PREFIX) ? "*" : "", pban->name,
             IS_SET(pban->ban_flags, BAN_SUFFIX) ? "*" : "");
    send_to_char(buf, ch);
}

void do_ban(CHAR_DATA *ch, char *argument) { ban_site(ch, argument, false); }

void do_permban(CHAR_DATA *ch, char *argument) { ban_site(ch, argument, true); }

void do_allow(CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH], *aargh = arg;
    char buf[MAX_STRING_LENGTH];
    BAN_DATA *prev;
    BAN_DATA *curr;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Remove which site from the ban list?\n\r", ch);
        return;
    }

    if (arg[0] == '*')
        aargh++;

    if (aargh[strlen(aargh)] == '*')
        aargh[strlen(aargh)] = '\0';

    prev = nullptr;
    for (curr = ban_list; curr != nullptr; prev = curr, curr = curr->next) {
        if (!str_cmp(aargh, curr->name)) {
            if (curr->level > get_trust(ch)) {
                send_to_char("You are not powerful enough to lift that ban.\n\r", ch);
                return;
            }
            if (prev == nullptr)
                ban_list = ban_list->next;
            else
                prev->next = curr->next;

            snprintf(buf, sizeof(buf), "Ban on '%s%s%s' lifted.\n\r", IS_SET(curr->ban_flags, BAN_PREFIX) ? "*" : "",
                     aargh, IS_SET(curr->ban_flags, BAN_SUFFIX) ? "*" : "");
            free_ban(curr);
            send_to_char(buf, ch);
            save_bans();
            return;
        }
    }

    send_to_char("That site is not banned.\n\r", ch);
}
