#include "merc.h"
#include "character.h"
#include "channels.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "interp.h"
#include "libstring.h"

#include <stdio.h>
#include <string.h>


extern unsigned int parse_unsigned_int(char *string);
extern void mp_act_trigger(char *argument, CHAR_DATA * mob, CHAR_DATA * ch, const void *arg1, const void *arg2, int type);
extern int parse_int(char *string);


void do_delet(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
	send_to_char("You must type the full command to delete yourself.\n\r", ch);

    return;
}

void do_delete(CHAR_DATA *ch, char *argument)
{
	char strsave[MIL];

	DENY_NPC(ch);

	if (ch->pcdata->confirm_delete) {
		if (argument[0] != '\0') {
			send_to_char("Delete status removed.\n\r", ch);
			ch->pcdata->confirm_delete = FALSE;
			(void)snprintf(log_buf, MAX_NAME_LENGTH + 32, "DELETE: %s just couldn't do it...", ch->name);
			log_string(log_buf);

			return;
		} else {
			char filename[MAX_NAME_LENGTH];

			filename[0] = '\0';

			capitalize_into(ch->name, (char *)filename, MAX_NAME_LENGTH);
			(void)snprintf(strsave, MIL, "%s%s", PLAYER_DIR, filename);

			wiznet("$N turns $Mself into line noise.", ch, NULL, 0, 0, 0);
			(void)snprintf(log_buf, MAX_NAME_LENGTH + 32, "DELETE: %s is TOAST! wOOp!", ch->name);
			log_string(log_buf);

			do_quit(ch, "");

			(void)unlink(strsave);
			do_echo(NULL, "You see a flash of `&lightning``.\n\r");
			return;
		}
	}

	if (argument[0] != '\0') {
		send_to_char("Just type delete. No argument.\n\r", ch);
		return;
	}

	send_to_char("Type delete again to confirm this command.\n\r", ch);
	send_to_char("```!WARNING``: this command is irreversible.\n\r", ch);
	send_to_char("Typing delete with an argument will undo delete status.\n\r", ch);

	ch->pcdata->confirm_delete = TRUE;

	wiznet("$N is contemplating deletion.", ch, NULL, 0, 0, get_trust(ch));
	(void)snprintf(log_buf, MAX_NAME_LENGTH + 64, "DELETE: %s .. %s's thinking about it ..", ch->name, ch->sex == 0 ? "It" : ch->sex == 1 ? "He" : "She");
	log_string(log_buf);

    return;
}


void do_fixscreen(CHAR_DATA *ch, char *argument)
{
	// fix the screen....send clear screen and set the number of lines
	int lines;

	if (argument[0] == '\0' || !is_number(argument))
		lines = 25;
	else
		lines = parse_int(argument);

	if ((lines < 1) || (lines > 100)) {
		send_to_char("Get real.\n\r", ch);
		return;
	}

	printf_to_char(ch, "\033[0;37;40m\033[%d;1f\033[2J\n\rScreen fixed.\n\r", lines);
	return;
}

void do_clearscreen(CHAR_DATA *ch, /*@unused@*/char *argument)
{
	send_to_char("\033[0H\033[2J\n", ch);
	return;
}

void do_channels(CHAR_DATA *ch, /*@unused@*/char *argument)
{
	if (IS_NPC(ch))
		return;

	send_to_char("CHANNEL        STATUS\n\r", ch);
	send_to_char("```&---------------------``\n\r", ch);

	printf_to_char(ch, "`![Info]         %s``\n\r",
		       IS_SET(ch->comm2, COMM2_INFO) ? "`#ON" : "`1OFF");

	printf_to_char(ch, "`3Auction        %s``\n\r",
		       !IS_SET(ch->comm, COMM_NOAUCTION) ? "`#ON" : "`1OFF");

	printf_to_char(ch, "`4Quote          %s``\n\r",
		       !IS_SET(ch->comm, COMM_NOQUOTE) ? "`#ON" : "`1OFF");

	printf_to_char(ch, "`1Shouts         %s``\n\r",
		       !IS_SET(ch->comm, COMM_SHOUTSOFF) ? "`#ON" : "`1OFF");

	printf_to_char(ch, "`#OOC            %s``\n\r",
		       !IS_SET(ch->comm, COMM_NOOOC) ? "`#ON" : "`1OFF");

	printf_to_char(ch, "`#QChat          %s``\n\r",
		       !IS_SET(ch->comm2, COMM2_NOQCHAT) ? "`#ON" : "`1OFF");

	printf_to_char(ch, "`!Trivia         %s``\n\r",
		       (!IS_SET(ch->comm, COMM_NOTRIVIA) && !IS_SET(ch->comm, COMM_TRIVIAOFF)) ? "`#ON" : "`1OFF");

	printf_to_char(ch, "`!Auto Trivia    %s``\n\r",
		       (!IS_SET(ch->comm2, COMM_TTRIV) && !IS_SET(ch->comm2, COMM_TTRIV)) ? "`#ON" : "`1OFF");

	printf_to_char(ch, "`@Tells          %s``\n\r",
		       !IS_SET(ch->comm, COMM_DEAF) ? "`#ON" : "`1OFF");

	printf_to_char(ch, "`^Quiet Mode     %s``\n\r",
		       IS_SET(ch->comm, COMM_QUIET) ? "`#ON" : "`1OFF");

	printf_to_char(ch, "`#Battlefield    %s``\n\r",
		       IS_SET(ch->comm2, COMM_NOBATTLEFIELD) ? "`#ON" : "`1OFF");

	if (IS_IMMORTAL(ch)) {
		printf_to_char(ch, "`!GOD channel    %s``\n\r",
			       !IS_SET(ch->comm, COMM_NOWIZ) ? "`#ON" : "`1OFF");

		printf_to_char(ch, "`OWishes         %s``\n\r",
			       !IS_SET(ch->comm, COMM_NOWISH) ? "`#ON" : "`1OFF");
	}

	if (ch->level == IMPLEMENTOR)
		printf_to_char(ch, "`2I`8M`2P            %s``\n\r",
			       !IS_SET(ch->comm2, COMM2_IMPTALK) ? "`#ON" : "`1OFF");

	send_to_char("\n\r", ch);
	if (IS_SET(ch->comm2, COMM2_AFK))
		send_to_char("You are ```!A```@F```OK``.\n\r", ch);

	if (IS_SET(ch->comm2, COMM2_BUSY))
		send_to_char("You are Busy.\n\r", ch);

	if (IS_SET(ch->comm2, COMM2_CODING))
		send_to_char("You are `@Coding``.\n\r", ch);

	if (IS_SET(ch->comm2, COMM2_BUILD))
		send_to_char("You are `3Building``.\n\r", ch);

	if (IS_SET(ch->comm, COMM_SNOOP_PROOF))
		send_to_char("You are immune to snooping.\n\r", ch);

	if (ch->lines != PAGELEN) {
		if (ch->lines > 0)
			printf_to_char(ch, "You display %d lines of scroll.\n\r", ch->lines + 2);
		else
			send_to_char("Scroll buffering is off.\n\r", ch);
	}

	if (ch->prompt != NULL)
		printf_to_char(ch, "Your current prompt is: %s\n\r", ch->prompt);

	if (IS_SET(ch->comm, COMM_NOSHOUT))
		send_to_char("You cannot ```1shout``.\n\r", ch);

	if (IS_SET(ch->comm, COMM_NOTELL))
		send_to_char("You cannot use ```@tell``.\n\r", ch);

	if (IS_SET(ch->comm, COMM_NOCHANNELS))
		send_to_char("You cannot use channels.\n\r", ch);

	if (IS_SET(ch->comm2, COMM2_NOEMOTE))
		send_to_char("You cannot show emotions.\n\r", ch);

    return;
}


void do_deaf(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
    toggle_comm(ch, COMM_DEAF);

    return;
}

void do_quiet(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
    toggle_comm(ch, COMM_QUIET);

    return;
}

void do_afk(CHAR_DATA *ch, char *argument)
{
    DENY_NPC(ch);
    toggle_afk(ch, argument);

    return;
}

void do_replay(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
	if (IS_NPC(ch)) {
		send_to_char("You can't replay.\n\r", ch);
		return;
	}

	page_to_char(buf_string(ch->pcdata->buffer), ch);
	clear_buf(ch->pcdata->buffer);

    return;
}

void do_auctalk(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
        toggle_comm(ch, COMM_NOAUCTION);
	} else {
        broadcast_auctalk(ch, argument);
	}
    return;
}

void do_ooc(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
        toggle_comm(ch, COMM_NOOOC);
	} else {
        broadcast_ooc(ch, argument);
	}
    return;
}

void do_immtalk(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
        toggle_comm(ch, COMM_NOWIZ);
	} else {
        broadcast_immtalk(ch, argument);
    }
	return;
}

void do_imptalk(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
		toggle_comm(ch, COMM2_IMPTALK);
	} else {
        broadcast_imptalk(ch, argument);
    }
	return;
}

void do_wish(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
		send_to_char("`OWish`7 for what?\n\r", ch);
	} else {
        broadcast_wish(ch, argument);
    }
	return;
}

void do_say(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
		send_to_char("```&Say`` what?\n\r", ch);
	} else {
        broadcast_say(ch, argument);
    }

	return;
}

void do_osay(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
		send_to_char("Say what?\n\r", ch);
	} else {
        broadcast_osay(ch, argument);
    }
	return;
}

void do_shout(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
        toggle_comm(ch, COMM_SHOUTSOFF);
	} else {
        broadcast_shout(ch, argument);
    }
    return;
}


/***************************************************************************
*	do_info
*
*	turn the info channel on/off - imms can use info for messages
***************************************************************************/
void do_info(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
        toggle_comm(ch, COMM2_INFO);
	} else {
        if (!IS_IMMORTAL(ch)) {
            send_to_char("Use '`#info``' to turn the info channel on or off.", ch);
            return;
        }
        broadcast_info(ch, argument);
    }
	return;
}


/***************************************************************************
*	do_tell
*
*	sent a private message to a character
***************************************************************************/
void do_tell(CHAR_DATA *ch, char *argument)
{
	static char arg[MIL];
	static char buf[MSL];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0' || argument[0] == '\0') {
		send_to_char("```@Tell`` whom what?\n\r", ch);
	} else {
        /* Can tell to PC's anywhere, but NPC's only in same room.
         *    -- Furey
         */
        if ((victim = get_char_world(ch, arg)) == NULL
               || (IS_NPC(victim) && victim->in_room != ch->in_room)) {
            send_to_char("They aren't here.\n\r", ch);
            return;
        }

        if (victim->desc == NULL && !IS_NPC(victim)) {
            act("$N seems to have misplaced $S link...try again later.", ch, NULL, victim, TO_CHAR);
            (void)snprintf(buf, 2 * MIL, "```@%s tells you '`t%s```@'``\n\r", PERS(ch, victim), argument);
            buf[0] = UPPER(buf[0]);
            add_buf(victim->pcdata->buffer, buf);
            return;
        }
        broadcast_tell(ch, victim, argument);
    }
    return;
}


/***************************************************************************
*	do_reply
*
*	reply to a tell
***************************************************************************/
void do_reply(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char buf[MSL];
	int pos;
	bool found = FALSE;

	if (IS_SET(ch->comm, COMM_NOTELL)) {
		send_to_char("Your message didn't get through.\n\r", ch);
		return;
	}

	if ((ch->in_room->vnum > 20924) && (ch->in_room->vnum < 20930)
	    && (ch->level < LEVEL_IMMORTAL)) {
		send_to_char("Not in jail....\n\r", ch);
		return;
	}

	if ((victim = ch->reply) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim->desc == NULL && !IS_NPC(victim)) {
		act("$N seems to have misplaced $S link...try again later.",
		    ch, NULL, victim, TO_CHAR);
		(void)snprintf(buf, 2 * MIL, "`@%s tells you '`r%s`@'``\n\r", PERS(ch, victim), argument);
		buf[0] = UPPER(buf[0]);
		add_buf(victim->pcdata->buffer, buf);
		return;
	}

	if (!IS_NPC(victim)) {
		for (pos = 0; pos < MAX_IGNORE; pos++) {
			if (victim->pcdata->ignore[pos] == NULL)
				break;
			if (!str_cmp(ch->name, victim->pcdata->ignore[pos]))
				found = TRUE;
		}
	}

	if (found) {
		act("$N seems to be ignoring you.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (!IS_IMMORTAL(ch) && !IS_AWAKE(victim)) {
		act("$E can't hear you.", ch, 0, victim, TO_CHAR);
		return;
	}

	if ((IS_SET(victim->comm, COMM_QUIET) || IS_SET(victim->comm, COMM_DEAF))
	    && !IS_IMMORTAL(ch) && !IS_IMMORTAL(victim)) {
		act_new("$E is not receiving ```@tells``.", ch, 0, victim, TO_CHAR, POS_DEAD, FALSE);
		return;
	}

	if (!IS_IMMORTAL(victim) && !IS_AWAKE(ch)) {
		send_to_char("In your dreams, or what?\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm2, COMM2_AFK)) {
		if (IS_NPC(victim)) {
			act_new("$E is `!A`@F`OK``, and not receiving tells.", ch, NULL, victim, TO_CHAR, POS_DEAD, FALSE);
			return;
		}

		act_new("$E is `!A`@F`OK``, but your tell will go through when $E returns.", ch, NULL, victim, TO_CHAR, POS_DEAD, FALSE);
		(void)snprintf(buf, 2 * MIL, "`@%s tells you '`r%s`@'``\n\r", PERS(ch, victim), argument);
		buf[0] = UPPER(buf[0]);
		add_buf(victim->pcdata->buffer, buf);
		return;
	}

	act("`@You tell $N '`r$t`@'``", ch, argument, victim, TO_CHAR);
	act_new("`@$n tells you '`r$t`@'``", ch, argument, victim, TO_VICT, POS_DEAD, FALSE);
	victim->reply = ch;
	return;
}


/***************************************************************************
*	do_yell
*
*	yell a message to everyone in the area
***************************************************************************/
void do_yell(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;

	if (IS_SET(ch->comm, COMM_NOSHOUT)) {
		send_to_char("You can't ```1yell``.\n\r", ch);
		return;
	}

	if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
		send_to_char("The Gods have revoked your channel priviledges.\n\r", ch);
		return;
	}

	if ((ch->in_room->vnum > 20924)
	    && (ch->in_room->vnum < 20930)
	    && (ch->level < LEVEL_IMMORTAL)) {
		send_to_char("Not in jail....\n\r", ch);
		return;
	}

	if (argument[0] == '\0') {
		send_to_char("```1Yell`` what?\n\r", ch);
		return;
	}


	act("`1You yell '`!$t`1'``", ch, argument, NULL, TO_CHAR);
	for (d = descriptor_list; d != NULL; d = d->next) {
		if (d->connected == CON_PLAYING
		    && d->character != ch
		    && d->character->in_room != NULL
		    && d->character->in_room->area == ch->in_room->area
		    && !IS_SET(d->character->comm, COMM_QUIET)) {
				act("`1$n yells '`!$t`1'``", ch, argument, d->character, TO_VICT);
		}
	}

	return;
}

/***************************************************************************
*	do_emote
*
*	kind of like a do-it-yourself social
***************************************************************************/
void do_emote(CHAR_DATA *ch, char *argument)
{
	if (!IS_NPC(ch) && IS_SET(ch->comm2, COMM2_NOEMOTE)) {
		send_to_char("You can't show your emotions.\n\r", ch);
		return;
	}



	if (argument[0] == '\0') {
		send_to_char("Emote what?\n\r", ch);
		return;
	}

	act_new("$n $T", ch, NULL, emote_parse(ch, argument), TO_ROOM, POS_RESTING, FALSE);
	act_new("$n $T", ch, NULL, emote_parse(ch, argument), TO_CHAR, POS_RESTING, FALSE);

	return;
}


/***************************************************************************
*	do_pmote
*
*	kind of like emote, but changes the targets name
***************************************************************************/
void do_pmote(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *vch;
	char *letter;
	char *name;
	char last[MIL];
	char temp[MSL];
	int matches = 0;

	if (!IS_NPC(ch) && IS_SET(ch->comm2, COMM2_NOEMOTE)) {
		send_to_char("You can't show your emotions.\n\r", ch);
		return;
	}


	if (argument[0] == '\0') {
		send_to_char("Emote what?\n\r", ch);
		return;
	}

	act("$n $t", ch, argument, NULL, TO_CHAR);

	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
		if (vch->desc == NULL || vch == ch)
			continue;

		if ((letter = strstr(argument, vch->name)) == NULL) {
			act_new("$N $t", vch, argument, ch, TO_CHAR, POS_RESTING, FALSE);
			continue;
		}

		strcpy(temp, argument);
		temp[strlen(argument) - strlen(letter)] = '\0';
		last[0] = '\0';
		name = vch->name;

		for (; *letter != '\0'; letter++) {
			if (*letter == '\'' && matches == (int)strlen(vch->name)) {
				strcat(temp, "r");
				continue;
			}

			if (*letter == 's' && matches == (int)strlen(vch->name)) {
				matches = 0;
				continue;
			}

			if (matches == (int)strlen(vch->name))
				matches = 0;

			if (*letter == *name) {
				matches++;
				name++;
				if (matches == (int)strlen(vch->name)) {
					strcat(temp, "you");
					last[0] = '\0';
					name = vch->name;
					continue;
				}
				strncat(last, letter, 1);
				continue;
			}

			matches = 0;
			strcat(temp, last);
			strncat(temp, letter, 1);
			last[0] = '\0';
			name = vch->name;
		}

		act_new("$N $t", vch, temp, ch, TO_CHAR, POS_RESTING, FALSE);
	}

	return;
}


void do_bug(CHAR_DATA *ch, char *argument)
{
	append_file(ch, BUG_FILE, argument);
	send_to_char("Bug logged.\n\r", ch);
	return;
}

void do_typo(CHAR_DATA *ch, char *argument)
{
	append_file(ch, TYPO_FILE, argument);
	send_to_char("Typo logged.\n\r", ch);
	return;
}

void do_rent(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
	send_to_char("There is no rent here.  Just save and quit.\n\r", ch);
	return;
}


void do_qui(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
	send_to_char("If you want to QUIT, you have to spell it out.\n\r", ch);
	return;
}



void do_quit(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
	char buf[MSL];
	char strsave[2*MIL];
	DESCRIPTOR_DATA *d, *d_next;
	long id;

	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->comm, COMM_NOQUIT)) {
		send_to_char("You're having too much fun!\n\r", ch);
		return;
	}

	if (!IS_NPC(ch) && ch->timer < 30) {
		if (ch->pnote != NULL) {
			send_to_char("You have some kind of note in progress.  Please clear it or post it before quitting.\n\r", ch);
			return;
		}
	}

	if (ch->pk_timer < 0) {
		send_to_char("You cannot quit with a PK timer!\n\r", ch); return;
	}

	if (IS_SET(ch->act, PLR_IT)) {
		send_to_char("You're it! You have to tag someone before you can quit.\n\r", ch);
		return;
	}

	if (ch->last_fight
	    && (current_time - ch->last_fight < 90)
	    && !ch->pcdata->confirm_delete) {
		send_to_char("You must wait 90 seconds after a pfight before you can quit.\n\r", ch);
		return;
	}

	if (ch->position == POS_FIGHTING) {
		send_to_char("No way! You are fighting.\n\r", ch);
		return;
	}

	if (ch->position < POS_STUNNED) {
		send_to_char("You're not DEAD yet.\n\r", ch);
		return;
	}

	if (auction->item != NULL && ((ch == auction->buyer) || (ch == auction->seller))) {
		(void)snprintf(buf, 2 * MIL, "Wait until the auctioning of %s is over.\n\r", auction->item->short_descr);
		send_to_char(buf, ch);
		return;
	}

	furniture_check(ch);

	send_to_char("`@Disconnected.``\n\r", ch);
	act("```@$n ```Ohas ```^left ```#the ```!game.``", ch, NULL, NULL, TO_ROOM);
	(void)snprintf(log_buf, 2 * MIL, "%s has quit.", ch->name);
	log_string(log_buf);
	wiznet("$N rejoins the real world.", ch, NULL, WIZ_LOGINS, 0, get_trust(ch));

    /*
     * After extract_char the ch is no longer valid!
     */
	if (IS_SET(ch->act, PLR_LINKDEAD))
		REMOVE_BIT(ch->act, PLR_LINKDEAD);
	affect_strip(ch, gsp_deft);
	affect_strip(ch, gsp_dash);
	affect_strip(ch, skill_lookup("blood rage"));

	save_char_obj(ch);
	id = ch->id;
	d = ch->desc;

	if ((ch->level == 1) && (!IS_SET(ch->act, PLR_DENY))) {
		(void)snprintf(strsave, 2 * MIL, "%s%s", PLAYER_DIR, capitalize(ch->name));
		wiznet("`!Killing`7: $N `1(`7Level 1 cleanup`1)`7", ch, NULL, 0, 0, 0);
		unlink(strsave);
	}
	extract_char(ch, TRUE);
	if (d != NULL)
		close_socket(d);

    /* toast evil cheating bastards */
	for (d = descriptor_list; d != NULL; d = d_next) {
		CHAR_DATA *tch;

		d_next = d->next;
		tch = d->original ? d->original : d->character;
		if (tch && tch->id == id) {
			extract_char(tch, TRUE);
			close_socket(d);
		}
	}

	return;
}



void do_save(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
	if (IS_NPC(ch))
		return;

	REMOVE_BIT(ch->act, PLR_LINKDEAD);
	save_char_obj(ch);
	send_to_char("```^You are saved... for now!!!``\n\r", ch);
	return;
}

void do_follow(CHAR_DATA *ch, char *argument)
{
    /* RT changed to allow unlimited following and follow the NOFOLLOW rules */
	CHAR_DATA *victim;
	char arg[MIL];

	(void)one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Follow whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL) {
		act("But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR);
		return;
	}

	if (victim == ch) {
		if (ch->master == NULL) {
			send_to_char("You already follow yourself.\n\r", ch);
			return;
		}
		stop_follower(ch);
		return;
	}

	if (!IS_NPC(victim) && IS_SET(victim->act, PLR_NOFOLLOW) && !IS_IMMORTAL(ch)) {
		act("$N doesn't seem to want any followers.\n\r", ch, NULL, victim, TO_CHAR);
		return;
	}

	REMOVE_BIT(ch->act, PLR_NOFOLLOW);

	if (ch->master != NULL)
		stop_follower(ch);

	add_follower(ch, victim);

	return;
}


void add_follower(CHAR_DATA *ch, CHAR_DATA *master)
{
	if (ch->master != NULL) {
		bug("Add_follower: non-null master.", 0);
		return;
	}

	ch->master = master;
	ch->leader = NULL;

	if (can_see(master, ch))
		act("$n now follows you.", ch, NULL, master, TO_VICT);

	act("You now follow $N.", ch, NULL, master, TO_CHAR);

	return;
}



void stop_follower(CHAR_DATA *ch)
{
	if (ch->master == NULL) {
		bug("Stop_follower: null master.", 0);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM)) {
		REMOVE_BIT(ch->affected_by, AFF_CHARM);
		affect_strip(ch, skill_lookup("charm person"));
	}

	if (can_see(ch->master, ch) && ch->in_room != NULL) {
		act("$n stops following you.", ch, NULL, ch->master, TO_VICT);
		act("You stop following $N.", ch, NULL, ch->master, TO_CHAR);
	}

	if (ch->master->pet == ch)
		ch->master->pet = NULL;

	ch->master = NULL;
	ch->leader = NULL;
	return;
}

/* nukes charmed monsters and pets */
void nuke_pets(CHAR_DATA *ch)
{
	CHAR_DATA *pet;

	if ((pet = ch->pet) != NULL) {
		stop_follower(pet);
		if (pet->in_room != NULL)
			act("$N pees on your leg and walks off.", ch, NULL, pet, TO_NOTVICT);

		extract_char(pet, TRUE);
	}
	ch->pet = NULL;

	return;
}



void die_follower(CHAR_DATA *ch)
{
	CHAR_DATA *fch;

	if (ch->master != NULL) {
		if (ch->master->pet == ch)
			ch->master->pet = NULL;
		stop_follower(ch);
	}

	ch->leader = NULL;

	for (fch = char_list; fch != NULL; fch = fch->next) {
		if (fch->master == ch)
			stop_follower(fch);
		if (fch->leader == ch)
			fch->leader = fch;
	}

	return;
}



void do_order(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	CHAR_DATA *och;
	CHAR_DATA *och_next;
	char buf[MSL];
	char arg[MIL];
	char arg2[MIL];
	bool found;
	bool fAll;

	argument = one_argument(argument, arg);
	(void)one_argument(argument, arg2);

	if (!str_cmp(arg2, "suicide")) {
		send_to_char("That will not be done..\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "bash")) {
		send_to_char("Sorry, no way.\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "delete") || !str_cmp(arg2, "mob")) {
		send_to_char("That will NOT be done.\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "quit")) {
		send_to_char("That will NOT be done.\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "brew")) {
		send_to_char("Sorry pal, ain't gonna happen.\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "scribe")) {
		send_to_char("Mobiles are not literate enough.\n\r", ch);
		return;
	}

	if (!str_prefix(arg2, "cast")) {
		send_to_char("Survey says:  No!\n\r", ch);
		return;
	}

	if (!str_prefix(arg2, "push")) {
		send_to_char("Survey says:  No!\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "pk")) {
		send_to_char("Huh? .. Kill what?\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "eat")) {
		send_to_char("Your friend is not hungry..\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "drink")) {
		send_to_char("Your friend is not thirsty..\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "enter")) {
		send_to_char("I'm sorry Dave, but I'm afraid I can't do that.\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "drop")) {
		send_to_char("No. Whore.\n\r", ch);
		return;
	}


	if (arg[0] == '\0' || argument[0] == '\0') {
		send_to_char("Order whom to do what?\n\r", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM)) {
		send_to_char("You feel like taking, not giving, orders.\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "all")) {
		fAll = TRUE;
		victim = NULL;
	} else {
		fAll = FALSE;
		if ((victim = get_char_room(ch, arg)) == NULL) {
			send_to_char("They aren't here.\n\r", ch);
			return;
		}

		if (victim == ch) {
			send_to_char("Aye aye, right away!\n\r", ch);
			return;
		}

		if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch
		    || (IS_IMMORTAL(victim) && victim->trust >= ch->trust)) {
			send_to_char("Do it yourself!\n\r", ch);
			return;
		}
	}

	found = FALSE;
	for (och = ch->in_room->people; och != NULL; och = och_next) {
		och_next = och->next_in_room;

		if (IS_AFFECTED(och, AFF_CHARM)
		    && och->master == ch
		    && (fAll || och == victim)) {
			found = TRUE;

			if (!str_infix(arg2, "rem")) {
				send_to_char("One at a time...\n\r", ch);
				return;
			}

			(void)snprintf(buf, 2 * MIL, "$n orders you to '%s'.", argument);
			act(buf, ch, NULL, och, TO_VICT);
			interpret(och, argument);
		}
	}

	if (found) {
		WAIT_STATE(ch, PULSE_VIOLENCE);
		send_to_char("Ok.\n\r", ch);
	} else {
		send_to_char("You have no followers here.\n\r", ch);
	}

	return;
}




void do_group(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	char arg[MIL];
	CHAR_DATA *victim;

	(void)one_argument(argument, arg);

	if (arg[0] == '\0') {
		CHAR_DATA *gch;
		CHAR_DATA *leader;

		leader = (ch->leader != NULL) ? ch->leader : ch;
		(void)snprintf(buf, MSL, "%s's group:\n\r", PERS(leader, ch));
		send_to_char(buf, ch);

		for (gch = char_list; gch != NULL; gch = gch->next) {
			if (is_same_group(gch, ch)) {
/*  THIS IS ALL COMPLETELY USELESS.  TODO - REWRITE IF I CARE.
 *      (void)snprintf(buf, MSL, "[%2d ", gch->level);
 *      if(IS_NPC(gch))
 *        (void)snprintf(buf, MSL, "%s] ", "Mob");
 *      else if((class_table[gch->class].name == "witch") &&(gch->sex == 1))
 *        (void)snprintf(buf, MSL, "%s] ", "Warl");
 *      else
 *        (void)snprintf(buf, MSL, "%s] ", class_table[gch->class].who_name);
 */
				(void)snprintf(buf, MSL, "%-5s `!%4d``/`1%4d`` hp `@%4d``/`2%4d`` mana `$%4d``/`4%4d`` mv `&%5d`` xp\n\r",
					       capitalize(PERS(gch, ch)),
					       gch->hit, gch->max_hit,
					       gch->mana, gch->max_mana,
					       gch->move, gch->max_move,
					       gch->exp);
				send_to_char(buf, ch);
			}
		}
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (ch->master != NULL || (ch->leader != NULL && ch->leader != ch)) {
		send_to_char("But you are following someone else!\n\r", ch);
		return;
	}

	if (victim->master != ch && ch != victim) {
		act("$N isn't following you.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_AFFECTED(victim, AFF_CHARM)) {
		send_to_char("You can't remove charmed mobs from your group.\n\r", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM)) {
		act("You like your master too much to leave $m!", ch, NULL, victim, TO_VICT);
		return;
	}

	if (is_same_group(victim, ch) && ch != victim) {
		victim->leader = NULL;
		act("$n removes $N from $s group.", ch, NULL, victim, TO_NOTVICT);
		act("$n removes you from $s group.", ch, NULL, victim, TO_VICT);
		act("You remove $N from your group.", ch, NULL, victim, TO_CHAR);
		return;
	}

	victim->leader = ch;
	act("$N joins $n's group.", ch, NULL, victim, TO_NOTVICT);
	act("You join $n's group.", ch, NULL, victim, TO_VICT);
	act("$N joins your group.", ch, NULL, victim, TO_CHAR);
	return;
}



/*
 * 'Split' originally by Gnort, God of Chaos.
 */


/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	char arg1[MIL], arg2[MIL];
	CHAR_DATA *gch;
	int members;
	unsigned int amount_gold = 0, amount_silver = 0;
	unsigned int share_gold, share_silver;
	unsigned int extra_gold, extra_silver;

	argument = one_argument(argument, arg1);
	(void)one_argument(argument, arg2);

	if (arg1[0] == '\0') {
		send_to_char("Split how much?\n\r", ch);
		return;
	}

	amount_silver = parse_unsigned_int(arg1);

	if (arg2[0] != '\0')
		amount_gold = parse_unsigned_int(arg2);

	if (amount_gold == 0 && amount_silver == 0) {
		send_to_char("You hand out zero coins, but no one notices.\n\r", ch);
		return;
	}

	if (ch->gold < amount_gold || ch->silver < amount_silver) {
		send_to_char("You don't have that much to split.\n\r", ch);
		return;
	}

	members = 0;
	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
		if (is_same_group(gch, ch) && !IS_AFFECTED(gch, AFF_CHARM))
			members++;

	if (members < 2) {
		send_to_char("Just keep it all.\n\r", ch);
		return;
	}

	share_silver = amount_silver / members;
	extra_silver = amount_silver % members;

	share_gold = amount_gold / members;
	extra_gold = amount_gold % members;

	if (share_gold == 0 && share_silver == 0) {
		send_to_char("Don't even bother, cheapskate.\n\r", ch);
		return;
	}

	ch->silver -= amount_silver;
	ch->silver += share_silver + extra_silver;
	ch->gold -= amount_gold;
	ch->gold += share_gold + extra_gold;

	if (share_silver > 0) {
		(void)snprintf(buf, 2 * MIL,
			       "You split %u silver coins. Your share is %u silver.\n\r",
			       amount_silver, share_silver + extra_silver);
		send_to_char(buf, ch);
	}

	if (share_gold > 0) {
		(void)snprintf(buf, 2 * MIL,
			       "You split %u gold coins. Your share is %u gold.\n\r",
			       amount_gold, share_gold + extra_gold);
		send_to_char(buf, ch);
	}

	if (share_gold == 0) {
		(void)snprintf(buf, 2 * MIL, "$n splits %u silver coins. Your share is %u silver.",
			       amount_silver, share_silver);
	} else if (share_silver == 0) {
		(void)snprintf(buf, 2 * MIL, "$n splits %u gold coins. Your share is %u gold.",
			       amount_gold, share_gold);
	} else {
		(void)snprintf(buf, 2 * MIL,
			       "$n splits %u silver and %u gold coins, giving you %u silver and %u gold.\n\r",
			       amount_silver, amount_gold, share_silver, share_gold);
	}

	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room) {
		if (gch != ch && is_same_group(gch, ch) && !IS_AFFECTED(gch, AFF_CHARM)) {
			act(buf, ch, NULL, gch, TO_VICT);
			gch->gold += share_gold;
			gch->silver += share_silver;
		}
	}

	return;
}



void do_gtell(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	CHAR_DATA *gch;

	if (argument[0] == '\0') {
		send_to_char("Tell your group what?\n\r", ch);
		return;
	}

	if (IS_SET(ch->comm, COMM_NOTELL)) {
		send_to_char("Your message didn't get through!\n\r", ch);
		return;
	}


/*
 * Note use of send_to_char, so gtell works on sleepers.
 */
	(void)snprintf(buf, 2 * MIL, "```5%s tells the group '```P%s```5'``\n\r", ch->name,
		       argument);
	for (gch = char_list; gch != NULL; gch = gch->next)
		if (is_same_group(gch, ch))
			send_to_char(buf, gch);

	return;
}



/*
 * It is very important that this be an equivalence relation:
 *(1) A ~ A
 *(2) if A ~ B then B ~ A
 *(3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group(CHAR_DATA *ach, CHAR_DATA *bch)
{
	if (ach == NULL || bch == NULL)
		return FALSE;

	if (IS_NPC(ach) && IS_NPC(bch)) {
		if (ach->group == bch->group)
			return TRUE;
	}

	if (ach->leader != NULL)
		ach = ach->leader;
	if (bch->leader != NULL)
		bch = bch->leader;

	return ach == bch;
}


void do_noauction(CHAR_DATA *ch, char *argument)
{
	if (argument[0] == '\0') {
		if (IS_SET(ch->comm, COMM_NOAUCTION)) {
			send_to_char("You will now see `#auctions`7.\n\r", ch);
			REMOVE_BIT(ch->comm, COMM_NOAUCTION);
		} else {
			send_to_char("You will no longer see `#auctions`7.\n\r", ch);
			SET_BIT(ch->comm, COMM_NOAUCTION);
		}
	}
}

void do_page(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MIL];
	char buf[MSL];

	if (IS_NPC(ch))
		return;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Page syntax: \n\r", ch);
		send_to_char("  page <character name>\n\r", ch);
		return;
	}

	if (!(victim = get_char_world(ch, arg))) {
		send_to_char("They are not here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("You can't page an NPC ..\n\r", ch);
		return;
	}

	(void)snprintf(buf, 2 * MIL, "\aYou page `O%s`7.\n\r", victim->name);
	send_to_char(buf, ch);
	(void)snprintf(buf, 2 * MIL, "\a`O%s`7 is paging you.\n\r", ch->name);
	send_to_char(buf, victim);
	return;
}

void do_ignor(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
	send_to_char("You must enter the full commanfd to ignore someone.\n\r", ch);
	return;
}

void do_ignore(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *rch;
	char arg[MIL], buf[MSL];
	DESCRIPTOR_DATA *d;
	int pos;
	bool found = FALSE;

	if (ch->desc == NULL)
		rch = ch;
	else
		rch = ch->desc->original ? ch->desc->original : ch;

	if (IS_NPC(rch))
		return;

	smash_tilde(argument);

	argument = one_argument(argument, arg);

	if (arg[0] == '\0') {
		if (rch->pcdata->ignore[0] == NULL) {
			send_to_char("You are not ignoring anyone.\n\r", ch);
			return;
		}
		send_to_char("You are currently ignoring:\n\r", ch);

		for (pos = 0; pos < MAX_IGNORE; pos++) {
			if (rch->pcdata->ignore[pos] == NULL)
				break;

			(void)snprintf(buf, 2 * MIL, "    %s\n\r", rch->pcdata->ignore[pos]);
			send_to_char(buf, ch);
		}
		return;
	}

	for (pos = 0; pos < MAX_IGNORE; pos++) {
		if (rch->pcdata->ignore[pos] == NULL)
			break;

		if (!str_cmp(arg, rch->pcdata->ignore[pos])) {
			send_to_char("You are already ignoring that person.\n\r", ch);
			return;
		}
	}

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		if (d->connected != CON_PLAYING || !can_see(ch, d->character))
			continue;

		wch = (d->original != NULL) ? d->original : d->character;

		if (!can_see(ch, wch))
			continue;

		if (!str_cmp(arg, wch->name)) {
			found = TRUE;
			if (wch == ch) {
				send_to_char("You try to ignore yourself .. and fail.\n\r", ch);
				return;
			}
			if (wch->level >= LEVEL_IMMORTAL) {
				send_to_char("That person is very hard to ignore.\n\r", ch);
				return;
			}
		}
	}

	if (!found) {
		send_to_char("No one by that name is playing.\n\r", ch);
		return;
	}

	for (pos = 0; pos < MAX_IGNORE; pos++)
		if (rch->pcdata->ignore[pos] == NULL)
			break;

	if (pos >= MAX_IGNORE) {
		send_to_char("Stop being so antisocial!\n\r", ch);
		return;
	}

	rch->pcdata->ignore[pos] = str_dup(arg);
	(void)snprintf(buf, 2 * MIL, "You are now deaf to %s.\n\r", arg);
	send_to_char(buf, ch);
}

void do_unignore(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *rch;
	char arg[MIL], buf[MSL];
	int pos;
	bool found = FALSE;

	if (ch->desc == NULL)
		rch = ch;
	else
		rch = ch->desc->original ? ch->desc->original : ch;

	if (IS_NPC(rch))
		return;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0') {
		if (rch->pcdata->ignore[0] == NULL) {
			send_to_char("You are not ignoring anyone.\n\r", ch);
			return;
		}
		send_to_char("You are currently ignoring:\n\r", ch);

		for (pos = 0; pos < MAX_IGNORE; pos++) {
			if (rch->pcdata->ignore[pos] == NULL)
				break;

			(void)snprintf(buf, 2 * MIL, "    %s\n\r", rch->pcdata->ignore[pos]);
			send_to_char(buf, ch);
		}
		return;
	}

	for (pos = 0; pos < MAX_IGNORE; pos++) {
		if (rch->pcdata->ignore[pos] == NULL)
			break;

		if (found) {
			rch->pcdata->ignore[pos - 1] = rch->pcdata->ignore[pos];
			rch->pcdata->ignore[pos] = NULL;
			continue;
		}

		if (!str_cmp(arg, rch->pcdata->ignore[pos])) {
			send_to_char("Ignore removed.\n\r", ch);
			free_string(rch->pcdata->ignore[pos]);
			rch->pcdata->ignore[pos] = NULL;
			found = TRUE;
		}
	}

	if (!found)
		send_to_char("You aren't ignoring anyone by that name!\n\r", ch);
}



/***************************************************************************
*	do_tag
*
*	say something over the auctalk channel
***************************************************************************/
void do_tag(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];
	char buf[MSL];
	CHAR_DATA *victim;

	if (!IS_SET(ch->act, PLR_IT)) {
		send_to_char("You're not it.\n\r", ch);
		return;
	}

	(void)one_argument(argument, arg);

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("You can't tag what's not there.\n\r", ch);
		return;
	}
	if (IS_NPC(victim)) {
		send_to_char("Can't tag NPC's, silly.\n\r", ch);
		return;
	}
	if (ch->in_room != victim->in_room) {
		send_to_char("They're not here. \n\r", ch);
		return;
	}
	if (victim == ch->pcdata->tagged_by) {
		send_to_char("No tag-backs.\n\r", ch);
		return;
	}
	if (ch == victim) {
		send_to_char("Tag yourself. Very funny. Ha ha. \n\r", ch);
		return;
	}
	REMOVE_BIT(ch->act, PLR_IT);
	SET_BIT(victim->act, PLR_IT);
	victim->pcdata->tag_ticks = 0;
	victim->pcdata->tagged_by = ch;
	ch->pcdata->tag_ticks = -1;
	ch->pcdata->tagged_by = NULL;
	(void)snprintf(buf, 2 * MIL, "%s tags you. You're IT, buddy!\n\r", ch->name);
	send_to_char(buf, victim);
	(void)snprintf(buf, 2 * MIL, "You tag %s. They're IT now!\n\r", victim->name);
	send_to_char(buf, ch);
}

/*****************************
*      Added by Tylor        *
*****************************/
void do_tagstop(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];
	char buf[MSL];
	CHAR_DATA *victim;

	(void)one_argument(argument, arg);

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("You have to specify the person thats IT.\n\r", ch);
		return;
	}
	if (ch == victim) {
		send_to_char("That would be a bad idea. Tag someone else first.\n\r", ch);
		return;
	}
	if (ch->in_room != victim->in_room) {
		send_to_char("They're not here.\n\r", ch);
		return;
	}

	REMOVE_BIT(victim->act, PLR_IT);
	victim->pcdata->tag_ticks = -1;
	victim->pcdata->tagged_by = NULL;
	(void)snprintf(buf, 2 * MIL, "%s has saved you from being tagged any longer. Sweet!\n\r", ch->name);
	send_to_char(buf, victim);
	(void)snprintf(buf, 2 * MIL, "You've untagged %s. Immtag has been stopped.\n\r", victim->name);
	send_to_char(buf, ch);
}


void do_rpswitch(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_IMMORTAL(ch)) {
		if (IS_SET(ch->comm2, COMM2_RP)) {
			send_to_char("You are now in `#OOC`` mode.\n\r", ch);
			act("$n is now in `#OOC`` mode.\n\r", ch, NULL, NULL, TO_ROOM);
			REMOVE_BIT(ch->comm2, COMM2_RP);
		} else {
			send_to_char("You are now in `!RP`` mode.\n\r", ch);
			act("$n is now in `RP`` mode.\n\r", ch, NULL, NULL, TO_ROOM);
			SET_BIT(ch->comm2, COMM2_RP);
		}
	} else {
		if (IS_SET(ch->comm2, COMM2_RP)) {
			send_to_char("You are now in `#OOC`` mode.\n\r", ch);
			act("$n is now in `8[`#OOC`8]`` mode.\n\r", ch, NULL, NULL, TO_ROOM);
			REMOVE_BIT(ch->comm2, COMM2_RP);
			SET_BIT(ch->comm2, COMM2_OOC);
			wiznet("$N has returned to `#OOC`` mode.", ch, NULL, WIZ_ROLEPLAY, 0, 0);
		} else {
			send_to_char("You are now in `!RP`` mode.  Enjoy your RP!\n\r", ch);
			act("$n is now in `8[`!RP`8]`` mode.\n\r", ch, NULL, NULL, TO_ROOM);
			REMOVE_BIT(ch->comm2, COMM2_OOC);
			SET_BIT(ch->comm2, COMM2_RP);
			wiznet("$N has entered `!RP`` mode.", ch, NULL, WIZ_ROLEPLAY, 0, 0);
		}
	}
	return;
}

void do_sayto(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MIL];
	int pos;
	bool found = FALSE;
	DESCRIPTOR_DATA *d;

	argument = one_argument(argument, arg);
	if (arg[0] == '\0' || argument[0] == '\0') {
		send_to_char("`8Say`` what to whom?\n\r", ch);
		return;
	}
	if ((victim = get_char_world(ch, arg)) == NULL
	    || victim->in_room != ch->in_room) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}
	if (victim->desc == NULL && !IS_NPC(victim)) {
		act("$N seems to have misplaced $S link...try again later.", ch, NULL, victim, TO_CHAR);
		return;
	}
	if (!IS_NPC(victim)) {
		for (pos = 0; pos < MAX_IGNORE; pos++) {
			if (victim->pcdata->ignore[pos] == NULL)
				break;
			if (!str_cmp(ch->name, victim->pcdata->ignore[pos]))
				found = TRUE;
		}
	}
	if (found) {
		if (!IS_IMMORTAL(ch)) {
			act("$N is not paying attention to you right now.", ch, NULL, victim, TO_CHAR);
			return;
		}
	}
	if (!(IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL) && !IS_AWAKE(victim)) {
		act("$E can't hear you.", ch, 0, victim, TO_CHAR);
		return;
	}
	if (IS_SET(victim->comm2, COMM2_AFK)) {
		if (IS_NPC(victim)) {
			act("$E is `!A`@F`OK``, and is unable to pay attention.", ch, NULL, victim, TO_CHAR);
			return;
		}
		return;
	}
	printf_to_char(ch, "``You say %sto %s '`P%s``'\n\r",
		       IS_SET(ch->comm2, COMM2_RP) ? "" : "`6(`&oocly`6)`` ",
		       victim->name, argument);

	printf_to_char(victim, "``%s says %sto you '`P%s``'\n\r",
		       ch->name,
		       IS_SET(ch->comm2, COMM2_RP) ? "" : "`6(`&oocly`6)`` ",
		       argument);

	for (d = descriptor_list; d; d = d->next) {
		if (d->connected == CON_PLAYING
		    && d->character->in_room == ch->in_room
		    && d->character != ch
		    && d->character->position != POS_SLEEPING
		    && d->character != victim) {
			if (!IS_NPC(victim)) {
				printf_to_char(d->character, "%s says %sto %s, '`P%s``'.\n\r",
					       PERS(ch, d->character),
					       IS_SET(ch->comm2, COMM2_RP) ? "" : "`6(`&oocly`6)`` ",
					       PERS(victim, d->character), argument);
			} else {
				printf_to_char(d->character, "%s says %sto %s, '`P%s``'.\n\r",
					       PERS(ch, d->character),
					       IS_SET(ch->comm2, COMM2_RP) ? "" : "`6(`&oocly`6)`` ",
					       PERS(victim, d->character), argument);
			}
		}
	}
	return;
}

char *emote_parse(/*@unused@*/ CHAR_DATA *ch, char *argument)
{
	static char target[MSL];
	char buf[MSL];
	int i = 0, arg_len = 0;
	int flag = FALSE;

	/* Reset target each time before use */
	memset(target, 0x00, sizeof(target));
	arg_len = (int)strlen(argument);
	for (i = 0; i < arg_len; i++) {
		if (argument[i] == '"' && flag == FALSE) {
			flag = TRUE;
			(void)snprintf(buf, MSL, "%c", argument[i]);
			strcat(target, buf);
			strcat(target, "`P");
			continue;
		} else if (argument[i] == '"' && flag == TRUE) {
			flag = FALSE;
			strcat(target, "``");
			(void)snprintf(buf, MSL, "%c", argument[i]);
			strcat(target, buf);
			continue;
		} else {
			(void)snprintf(buf, MSL, "%c", argument[i]);
			strcat(target, buf);
		}
	}
	if (flag == TRUE) {
		strcat(target, "``");
		(void)snprintf(buf, MSL, "%c", '"');
		strcat(target, buf);
	}
	return target;
}

void do_busy(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
	if (IS_SET(ch->comm2, COMM2_BUSY)) {
		send_to_char("Busy flag removed. Type 'replay' to see tells.\n\r", ch);
		REMOVE_BIT(ch->comm2, COMM2_BUSY);
	} else {
		send_to_char("You are now marked as busy.\n\r", ch);
		SET_BIT(ch->comm2, COMM2_BUSY);
	}
}

void do_coding(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
	if (IS_SET(ch->comm2, COMM2_CODING)) {
		send_to_char("Coding flag removed. Type 'replay' to see tells.\n\r", ch);
		REMOVE_BIT(ch->comm2, COMM2_CODING);
	} else {
		send_to_char("You are now marked as `@Coding``.\n\r", ch);
		SET_BIT(ch->comm2, COMM2_CODING);
	}
}

void do_building(CHAR_DATA *ch, /*@unused@*/ char *argument)
{
	if (IS_SET(ch->comm2, COMM2_BUILD)) {
		send_to_char("Building flag removed. Type 'replay' to see tells.\n\r", ch);
		REMOVE_BIT(ch->comm2, COMM2_BUILD);
	} else {
		send_to_char("You are now marked as `3Building``.\n\r", ch);
		SET_BIT(ch->comm2, COMM2_BUILD);
	}
}
