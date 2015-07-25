/************************************************************************
 *   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,         *
 *   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *	                                                                       *
 *   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael           *
 *   Chastain, Michael Quan, and Mitchell Tse.                              *
 *	                                                                       *
 *   In order to use any part of this Merc Diku Mud, you must comply with   *
 *   both the original Diku license in 'license.doc' as well the Merc	   *
 *   license in 'license.txt'.  In particular, you may not remove either of *
 *   these copyright notices.                                               *
 *                                                                             *
 *   Much time and thought has gone into this software and you are          *
 *   benefitting.  We hope that you share your changes too.  What goes      *
 *   around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*   ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*   ROM has been brought to you by the ROM consortium                      *
*       Russ Taylor(rtaylor@hypercube.org)                                 *
*       Gabrielle Taylor(gtaylor@hypercube.org)                            *
*       Brian Moore(zump@rom.org)                                          *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "interp.h"

#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>

/***************************************************************************
*	command procedures needed
***************************************************************************/
DECLARE_DO_FUN(do_pftell);
DECLARE_DO_FUN(do_gftell);
DECLARE_DO_FUN(do_grestore);
DECLARE_DO_FUN(do_rrestore);

extern long top_mob_index;
extern long top_obj_index;
extern bool is_space(const char test);
extern bool is_number(const char *test);

/***************************************************************************
*	local functions
***************************************************************************/
ROOM_INDEX_DATA *find_location(CHAR_DATA * ch, char *arg);

void reset_area(AREA_DATA * pArea);
void fry_char(CHAR_DATA * ch, char *argument);
void immkiss_char(CHAR_DATA * ch, CHAR_DATA * vch);
bool add_alias(CHAR_DATA * ch, char *alias, char *cmd);

bool set_char_hunger(CHAR_DATA * ch, CHAR_DATA * vch, char *argument);
bool set_char_thirst(CHAR_DATA * ch, CHAR_DATA * vch, char *argument);
bool set_char_feed(CHAR_DATA * ch, CHAR_DATA * vch, char *argument);
bool write_to_descriptor(int desc, char *txt, int length);

void sick_harvey_proctor(CHAR_DATA *ch, enum e_harvey_proctor_is, const char *message);

/*
 * External vars
 */
extern AFFECT_DATA *affect_free;



/***************************************************************************
*	do_wiznet
***************************************************************************/
void do_wiznet(CHAR_DATA *ch, char *argument)
{
	BUFFER *buf;
	long flag;

	DENY_NPC(ch)

	if (argument[0] == '\0') {
		if (IS_SET(ch->pcdata->wiznet, WIZ_ON))
			do_wiznet(ch, "off");
		else
			do_wiznet(ch, "on");
		return;
	}

	if (!str_prefix(argument, "on")) {
		send_to_char("Welcome to `^W`@i`^znet``\n\r", ch);
		SET_BIT(ch->pcdata->wiznet, WIZ_ON);
		return;
	}

	if (!str_prefix(argument, "off")) {
		send_to_char("Signing off of ```^W```@i```^znet``.\n\r", ch);
		REMOVE_BIT(ch->pcdata->wiznet, WIZ_ON);
		return;
	}

	/* show wiznet status */
	if (!str_prefix(argument, "status")) {
		buf = new_buf();
		if (!IS_SET(ch->pcdata->wiznet, WIZ_ON))
			add_buf(buf, "off ");

		for (flag = 0; wiznet_table[flag].name != NULL; flag++) {
			if (IS_SET(ch->pcdata->wiznet, wiznet_table[flag].flag)) {
				add_buf(buf, wiznet_table[flag].name);
				add_buf(buf, " ");
			}
		}

		add_buf(buf, "\n\r");

		send_to_char("```^W```@i```^znet`` status```8:``\n\r", ch);
		send_to_char(buf_string(buf), ch);

		free_buf(buf);
		return;
	}

	/* list of all wiznet options */
	if (!str_prefix(argument, "show")) {
		buf = new_buf();
		for (flag = 0; wiznet_table[flag].name != NULL; flag++) {
			if (wiznet_table[flag].level <= get_trust(ch)) {
				add_buf(buf, wiznet_table[flag].name);
				add_buf(buf, " ");
			}
		}

		add_buf(buf, "\n\r");

		send_to_char("```^W```@i```^znet`` options available to you are```8:``\n\r", ch);
		send_to_char(buf_string(buf), ch);

		free_buf(buf);
		return;
	}

	flag = wiznet_lookup(argument);
	if (flag == -1 || get_trust(ch) < wiznet_table[flag].level) {
		send_to_char("No such option.\n\r", ch);
		return;
	}

	if (IS_SET(ch->pcdata->wiznet, wiznet_table[flag].flag)) {
		printf_to_char(ch,
			       "You will no longer see %s on ```^W```@i```^znet``.\n\r",
			       wiznet_table[flag].name);
		REMOVE_BIT(ch->pcdata->wiznet, wiznet_table[flag].flag);
		return;
	} else {
		printf_to_char(ch,
			       "You will now see %s on ```^W```@i```^znet``.\n\r",
			       wiznet_table[flag].name);
		SET_BIT(ch->pcdata->wiznet, wiznet_table[flag].flag);
		return;
	}
}

/***************************************************************************
*	wiznet
***************************************************************************/
void wiznet(char *string, /*@null@*/ CHAR_DATA *ch, /*@null@*/ OBJ_DATA *obj, long flag, long flag_skip, int min_level)
{
	DESCRIPTOR_DATA *d;

	for (d = descriptor_list; d != NULL; d = d->next) {
		if (d->connected == CON_PLAYING
		    && !IS_NPC(d->character)
		    && IS_IMMORTAL(d->character)
		    && IS_SET(d->character->pcdata->wiznet, WIZ_ON)
		    && (!flag || IS_SET(d->character->pcdata->wiznet, flag))
		    && (!flag_skip || !IS_SET(d->character->pcdata->wiznet, flag_skip))
		    && get_trust(d->character) >= min_level
		    && d->character != ch) {
			if (IS_SET(d->character->pcdata->wiznet, WIZ_PREFIX))
				send_to_char("--> ", d->character);

			act_new(string, d->character, obj, ch, TO_CHAR, POS_DEAD, FALSE);
		}
	}

	return;
}


/***************************************************************************
*	do_impnet
*
*	bw[00]p!  ..  IMPNet -- aka Ripped and Mod'd Wiznet .. .
*	dont have a clue why this was imped vs. just adding more wiznet
*	flags, but here it is -- it is bug-free(?) and non-harmful.
***************************************************************************/
void do_impnet(CHAR_DATA *ch, char *argument)
{
	BUFFER *buf;
	long flag;

	DENY_NPC(ch)

	if (argument[0] == '\0') {
		if (IS_SET(ch->pcdata->impnet, IMN_ON))
			do_impnet(ch, "off");
		else
			do_impnet(ch, "on");
		return;
	}

	if (!str_prefix(argument, "on")) {
		send_to_char("Loading `PB`5ad `@T`2rip's `2I`8M`2P`7Net ..\n\r", ch);
		SET_BIT(ch->pcdata->impnet, IMN_ON);
		return;
	}

	if (!str_prefix(argument, "off")) {
		send_to_char("Signing off of `2I`8M`2P`7Net``.\n\r", ch);
		REMOVE_BIT(ch->pcdata->impnet, IMN_ON);
		return;
	}

	if (!str_prefix(argument, "status")) {
		buf = new_buf();

		if (!IS_SET(ch->pcdata->impnet, IMN_ON))
			add_buf(buf, "off ");

		for (flag = 0; impnet_table[flag].name != NULL; flag++) {
			if (IS_SET(ch->pcdata->impnet, impnet_table[flag].flag)) {
				add_buf(buf, impnet_table[flag].name);
				add_buf(buf, " ");
			}

			add_buf(buf, "\n\r");

			send_to_char("`2I`8M`2P`7Net status`8:``\n\r", ch);
			send_to_char(buf_string(buf), ch);

			free_buf(buf);
			return;
		}
	}

	if (!str_prefix(argument, "show")) {
		buf = new_buf();
		for (flag = 0; impnet_table[flag].name != NULL; flag++) {
			if (impnet_table[flag].level <= get_trust(ch)) {
				add_buf(buf, impnet_table[flag].name);
				add_buf(buf, " ");
			}
		}

		add_buf(buf, "\n\r");

		send_to_char("`2I`8M`2P`7Net options available are`8:``\n\r", ch);
		send_to_char(buf_string(buf), ch);

		free_buf(buf);
		return;
	}

	flag = impnet_lookup(argument);
	if (flag == -1 || get_trust(ch) < impnet_table[flag].level) {
		send_to_char("No such option.\n\r", ch);
		return;
	}

	if (IS_SET(ch->pcdata->impnet, impnet_table[flag].flag)) {
		printf_to_char(ch,
			       "You will no longer see `2%s`7 on `2I`8M`2P`7Net``.\n\r",
			       impnet_table[flag].name);
		REMOVE_BIT(ch->pcdata->impnet, impnet_table[flag].flag);
		return;
	} else {
		printf_to_char(ch,
			       "You will now see `2%s`7 on `2I`8M`2P`7Net``.\n\r",
			       impnet_table[flag].name);
		SET_BIT(ch->pcdata->impnet, impnet_table[flag].flag);
		return;
	}
}

/***************************************************************************
*	impnet
***************************************************************************/
void impnet(char *string, CHAR_DATA *ch, OBJ_DATA *obj, long flag, long flag_skip, int min_level)
{
	DESCRIPTOR_DATA *d;

	for (d = descriptor_list; d != NULL; d = d->next) {
		if (d->connected
		    && d->connected == CON_PLAYING
		    && !IS_NPC(d->character)
		    && IS_IMMORTAL(d->character)
		    && IS_SET(d->character->pcdata->impnet, IMN_ON)
		    && (!flag || IS_SET(d->character->pcdata->impnet, flag))
		    && (!flag_skip || !IS_SET(d->character->pcdata->impnet, flag_skip))
		    && get_trust(d->character) >= min_level
		    && d->character != ch) {
			if (IS_SET(d->character->pcdata->impnet, IMN_PREFIX))
				send_to_char("--> ", d->character);

			act_new(string, d->character, obj, ch, TO_CHAR, POS_DEAD, FALSE);
		}
	}

	return;
}






/***************************************************************************
*	do_jail
*
*	put a character in jail
***************************************************************************/
void do_jail(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char target[MIL];
	char duration[MIL];
	int location;
	int value;
	static int jail = 20925;

	if ((jail < 20925) || (jail > 20929))
		jail = 20925;

	location = jail;
	if (jail == 20929)
		jail = 20925;
	else
		jail++;

	argument = one_argument(argument, target);
	argument = one_argument(argument, duration);
	if ((target[0] == '\0') || (duration[0] == '\0')) {
		send_to_char("Syntax: jail <target> <BadTrip days>\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, target)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (!is_number(duration)) {
		send_to_char("Jail time must be a numeric representation of days.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Only on players.\n\r", ch);
		return;
	}
	value = atoi(duration);

	if (value < 0) {
		send_to_char("Jail time must be positive.\n\r", ch);
		return;
	}

	if (value == 0) {
		if (victim->pcdata->jail_release != 0) {
			victim->pcdata->jail_time = 0;
			send_to_char("Removing character from jail.\n\r", ch);
			send_to_char("You have been freed from jail.\n\r", victim);
			char_from_room(victim);
			char_to_room(victim, get_room_index(victim->pcdata->jail_release));
			return;
		} else {
			send_to_char("Character return room not found.\n\r", ch);
			return;
		}
	}

	sprintf(log_buf, "JAIL: %s jailed by %s for %d days.", victim->name, ch->name, value);
	log_string(log_buf);
	if (victim->fighting != NULL)
		stop_fighting(victim, TRUE);
	act("$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM);

	if ((victim->in_room->vnum < 20925) || (victim->in_room->vnum > 20929))
		victim->pcdata->jail_release = victim->in_room->vnum;
	else if (victim->pcdata->jail_release == 0)
		victim->pcdata->jail_release = 1;

	victim->pcdata->jail_time = value * 24;
	char_from_room(victim);
	char_to_room(victim, get_room_index(location));
	act("$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM);
	if (ch != victim)
		act("$n has jailed you.", ch, NULL, victim, TO_VICT);

	do_look(victim, "auto");
	send_to_char("Ok.\n\r", ch);
	save_char_obj(victim);
	return;
}



/***************************************************************************
*	do_tick
*
*	force a tick
***************************************************************************/
void do_tick(CHAR_DATA *ch, char *argument)
{
	extern bool tickset;

	DENY_NPC(ch)

	tickset = !tickset;
	if (tickset)
		wiznet("$N forces a TICK!.", ch, NULL, 0, 0, 0);

	return;
}

/***************************************************************************
*	do_grant
*
*	grant a skill/spell to a player
***************************************************************************/
void do_grant(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	SKILL *skill;
	LEARNED *learned;
	LEARNED *learned_found;
	char arg[MIL];

	DENY_NPC(ch)

	argument = one_argument(argument, arg);

	if (arg[0] == '\0' || argument[0] == '\0') {
		send_to_char("Syntax:\n\r", ch);
		send_to_char("       Grant [person] [skill/spell]\n\r", ch);
		return;
	}

	if (((victim = get_char_world(ch, arg)) == NULL)
	    || IS_NPC(ch)) {
		send_to_char("That person can't be found on heaven, hell, or earth\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Mobs were made with the skills they need,no more,no less.\n\r", ch);
		return;
	}

	if (!str_cmp(argument, "imptalk") && ch->level >= 610) {
		if (IS_SET(victim->comm2, COMM2_IMPTALKM)) {
			REMOVE_BIT(victim->comm2, COMM2_IMPTALKM);
			send_to_char("You may no longer use imptalk.\n\r", victim);
			send_to_char("You remove their imptalk abilities.\n\r", ch);
			return;
		} else {
			SET_BIT(victim->comm2, COMM2_IMPTALKM);
			send_to_char("You may now use the imptalk channel.\n\r", victim);
			send_to_char("You enable their abilities to use imptalk.\n\r", ch);
			return;
		}
	}

	if ((skill = skill_lookup(argument)) == NULL) {
		send_to_char("No such skill/spell\n\r", ch);
		return;
	}


	if ((learned_found = get_learned_skill(victim, skill)) != NULL
	    && learned_found->percent >= 75) {
		printf_to_char(ch, "%s already knows %s at %d%%.\n\r", victim->name, skill->name, learned_found->percent);
		return;
	}

	printf_log("%s granted %s by %s", victim->name, skill->name, ch->name);

	if (victim != ch)
		printf_to_char(victim, "You have been found worthy of %s!\n\r", skill->name);

	printf_to_char(ch, "You have granted %s %s!\n\r", victim->name, skill->name);

	learned = new_learned();
	if (learned != NULL) {
		learned->skill = skill;
		learned->percent = 75;
		learned->type = LEARNED_TYPE_SKILL;

		add_learned_skill(victim, learned);
	}
}




/***************************************************************************
*	do_outfit
*
*	equips a character
***************************************************************************/
void do_outfit(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	CHAR_DATA *vch;
	int idx;
	const struct newbie_eq_type {
		int	vnum;
		int	loc;
	} newbie_eq[] = {
		{ 3703, WEAR_BODY     },
		{ 3704, WEAR_SHIELD   },
		{ 3705, WEAR_NECK_1   },
		{ 3705, WEAR_NECK_2   },
		{ 3706, WEAR_HEAD     },
		{ 3707, WEAR_LEGS     },
		{ 3708, WEAR_FEET     },
		{ 3709, WEAR_HANDS    },
		{ 3710, WEAR_ARMS     },
		{ 3711, WEAR_ABOUT    },
		{ 3712, WEAR_WAIST    },
		{ 3713, WEAR_WRIST_R  },
		{ 3713, WEAR_WRIST_L  },
		{ 3715, WEAR_HOLD     },
		{ 3716, WEAR_LIGHT    },
		{ 3724, WEAR_FINGER_L },
		{ 3724, WEAR_FINGER_R },
		{ -1,	-1	      }
	};

	DENY_NPC(ch);

	if (IS_IMMORTAL(ch)) {
		if ((vch = get_char_world(ch, argument)) == NULL) {
			send_to_char("You cannot find that person.\n\r", ch);
			return;
		}

		if (IS_NPC(vch)) {
			send_to_char("Mobs dont care about your crappy newbie eq.\n\r", ch);
			return;
		}
	} else {
		vch = ch;
	}

	if (vch == NULL) {
		send_to_char("That person cannot be outfitted.\n\r", ch);
		return;
	}

	for (idx = 0; newbie_eq[idx].vnum > 0; idx++) {
		if ((obj = get_eq_char(vch, newbie_eq[idx].loc)) == NULL) {
			obj = create_object(get_obj_index(newbie_eq[idx].vnum), 0);
			obj->cost = 0;

			obj_to_char(obj, vch);
			equip_char(vch, obj, newbie_eq[idx].loc);
		}
	}

	if ((obj = get_eq_char(vch, WEAR_WIELD)) == NULL) {
		LEARNED *learned;
		int iter;
		long vnum;
		int pcnt;

		vnum = OBJ_VNUM_SCHOOL_SWORD;

		pcnt = 0;
		for (iter = 0; weapon_table[iter].name != NULL; iter++) {
			if ((learned = get_learned_skill(vch, *weapon_table[iter].gsp)) != NULL
			    && learned->percent >= 1) {
				if (learned->percent > pcnt) {
					vnum = weapon_table[iter].vnum;
					pcnt = learned->percent;
				}
			}
		}

		obj = create_object(get_obj_index(vnum), 0);

		obj_to_char(obj, vch);
		equip_char(vch, obj, WEAR_WIELD);
	}

	if (vch == ch)
		send_to_char("You have been equipped by Mota.\n\r", vch);
	else
		printf_to_char(vch, "You have been equipped by %s.\n\r", ch->name);
}


/***************************************************************************
*	do_nochannels
*
*	restricts channel access to a character
***************************************************************************/
void do_nochannels(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MIL];
	char buf[MSL];

	DENY_NPC(ch)

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Nochannel whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm, COMM_NOCHANNELS)) {
		REMOVE_BIT(victim->comm, COMM_NOCHANNELS);
		send_to_char("The gods have restored your channel priviliges.\n\r", victim);
		send_to_char("NOCHANNELS removed.\n\r", ch);

		sprintf(buf, "$N restores channels to %s", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->comm, COMM_NOCHANNELS);
		send_to_char("The gods have revoked your channel priviliges.\n\r", victim);
		send_to_char("NOCHANNELS set.\n\r", ch);

		sprintf(buf, "$N revokes %s's channels.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	}

	return;
}

/***************************************************************************
*	do_smote
*
*	emote with some personal funk to it
***************************************************************************/
void do_smote(CHAR_DATA *ch, char *argument)
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

	if (strstr(argument, ch->name) == NULL) {
		send_to_char("You must include your name in an smote.\n\r", ch);
		return;
	}

	send_to_char(argument, ch);
	send_to_char("\n\r", ch);

	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
		if (vch->desc == NULL || vch == ch)
			continue;

		if ((letter = strstr(argument, vch->name)) == NULL) {
			send_to_char(argument, vch);
			send_to_char("\n\r", vch);
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

		send_to_char(temp, vch);
		send_to_char("\n\r", vch);
	}

	return;
}

/***************************************************************************
*	do_bamfin
***************************************************************************/
void do_bamfin(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];

	if (!IS_NPC(ch)) {
		smash_tilde(argument);

		if (argument[0] == '\0') {
			sprintf(buf, "Your poofin is %s\n\r", ch->pcdata->bamfin);
			send_to_char(buf, ch);
			return;
		}

		free_string(ch->pcdata->bamfin);
		ch->pcdata->bamfin = str_dup(argument);

		sprintf(buf, "Your poofin is now `8:``%s\n\r", ch->pcdata->bamfin);
		send_to_char(buf, ch);
	}

	return;
}


/***************************************************************************
*	do_bamfout
***************************************************************************/
void do_bamfout(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];

	if (!IS_NPC(ch)) {
		smash_tilde(argument);

		if (argument[0] == '\0') {
			sprintf(buf, "Your poofout is `8:``%s\n\r", ch->pcdata->bamfout);
			send_to_char(buf, ch);
			return;
		}

		free_string(ch->pcdata->bamfout);
		ch->pcdata->bamfout = str_dup(argument);

		sprintf(buf, "Your poofout is now `8:``%s\n\r", ch->pcdata->bamfout);
		send_to_char(buf, ch);
	}

	return;
}


/***************************************************************************
*	do_deny
***************************************************************************/
void do_deny(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MIL];
	char buf[MSL];

	one_argument(argument, arg);

	DENY_NPC(ch)

	if (arg[0] == '\0') {
		send_to_char("Deny whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	SET_BIT(victim->act, PLR_DENY);
	send_to_char("`1O`!hhhh`8, `4d`$e`4n`$i`4e`$d`8! `2B`@am`2F\n\r", victim);


	sprintf(buf, "$N denies access to %s", victim->name);
	wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);

	send_to_char("OK.\n\r", ch);
	save_char_obj(victim);
	stop_fighting(victim, TRUE);
	do_quit(victim, "");
	return;
}

void do_bounty(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		send_to_char
			("Place what bounty on who's head?\n\rSyntax: Bounty <victim> <amount>\n\r",
			ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("They are currently not logged in!", ch);
		return;
	}

	if (ch == victim) {
		send_to_char("You cannot place bounties on yourself!\n\r", ch);
		return;
	}


	if (IS_NPC(victim)) {
		send_to_char("You cannot put a bounty on NPCs!\n\r", ch);
		return;
	}

	if (IS_IMMORTAL(ch) && !str_cmp(arg2, "clear")) {
		victim->pcdata->bounty = 0;
		sprintf(buf, "You have cleared %s's bounty.\n\r", victim->name);
		send_to_char(buf, ch);
		sprintf(buf, "Your bounty has been cleared.\n\r");
		send_to_char(buf, victim);
		sprintf(buf, "`![`7Bounty Info`!]`& %s's bounty has been cleared.`n", victim->name);
		do_echo(NULL, buf);
		return;
	}

	if (IS_IMMORTAL(victim)) {
		send_to_char("You cannot place bounties on gods!\n\r", ch);
		return;
	}

	if (is_number(arg2)) {
		int parsed_arg;
		unsigned int amount;
		parsed_arg = atoi(arg2);

		amount = parsed_arg <= 0 ? 1u : (unsigned int)parsed_arg;

		if (ch->pcdata->imp < amount) {
			send_to_char("You don't have that many imps!\n\r", ch);
			return;
		}

		ch->pcdata->imp -= amount;
		victim->pcdata->bounty += amount;
		sprintf(buf, "You have placed a %u imp bounty on %s.\n\r", amount, victim->name);
		send_to_char(buf, ch);
		sprintf(buf, "You are now worth %u imp(s).\n\r", victim->pcdata->bounty);
		send_to_char(buf, victim);
		sprintf(buf, "`![`7Bounty Info`!]`& %s `7is now worth `#%u `5imp`7(s).`n", victim->name, victim->pcdata->bounty);
		do_echo(NULL, buf);
		return;
	}
	send_to_char("The amount has to be a number\n\rSyntax: Bounty <victim> <value>.\n\r", ch);
	return;
}


void do_bountylist(CHAR_DATA *ch, char *argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *gch;


	one_argument(argument, arg);

	if (IS_NPC(ch))
		return;

	send_to_char("[       Name       ] [   Bounty   ]\n\r", ch);
	for (gch = char_list; gch != NULL; gch = gch->next) {
		if (IS_NPC(gch))
			continue;
		if (IS_IMMORTAL(gch))
			continue;
		if (gch->pcdata->bounty == 0)
			continue;

		sprintf(buf, "[%-18s] [ %10u ]\n\r", capitalize(gch->name), gch->pcdata->bounty);
		send_to_char(buf, ch);
	}
	return;
}


/***************************************************************************
*	do_disconnect
***************************************************************************/
void do_disconnect(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;
	char arg[MIL];

	DENY_NPC(ch)

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Disconnect whom?\n\r", ch);
		return;
	}
#if 0
	if (is_number(arg)) {
		int desc;

		desc = atoi(arg);
		for (d = descriptor_list; d != NULL; d = d->next) {
			if (ch->level <= victim->level) {
				send_to_char("You failed.\n\r", ch);
				return;
			}
			if (victim->desc == NULL) {
				act("$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR);
				return;
			}
			if (d->descriptor == desc) {
				close_socket(d);
				send_to_char("Ok.\n\r", ch);
				return;
			}
		}
	}
#endif

	victim = get_char_world(ch, arg);
	if (victim == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	} else if (is_number(arg)) {
		int desc;

		desc = atoi(arg);
		for (d = descriptor_list; d != NULL; d = d->next) {
			if ((d->descriptor == (SOCKET)desc) && (ch->level > victim->level)) {
				close_socket(d);
				send_to_char("Ok.\n\r", ch);
				return;
			} else if (ch->level <= victim->level) {
				send_to_char("You failed.\n\r", ch);
				return;
			}
		}
	}


	if (victim->desc == NULL) {
		act("$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR);
		return;
	}

	for (d = descriptor_list; d != NULL; d = d->next) {
		if ((d == victim->desc) && (ch->level > victim->level)) {
			close_socket(d);
			send_to_char("Ok.\n\r", ch);
			return;
		} else if (ch->level <= victim->level) {
			send_to_char("You failed.\n\r", ch);
			return;
		}
	}

	bug("do_disconnect: desc not found.", 0);
	send_to_char("Descriptor not found!\n\r", ch);
	return;
}

void do_chown(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	char arg1[MIL];
	char arg2[MIL];

	DENY_NPC(ch)

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		send_to_char("Get what from whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg2)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}
	if (!IS_NPC(victim) && (victim->level >= ch->level)) {
		send_to_char("I don't think they'd like that too much.\n\r", ch);
		return;
	}
	if ((obj = get_obj_carry(victim, arg1)) == NULL
	    && (obj = get_obj_wear(victim, arg1)) == NULL) {
		send_to_char("They do not have that item.\n\r", ch);
		return;
	}

	obj_from_char(obj);
	obj_to_char(obj, ch);

	act("$n makes a magical gesture and $p flys from $N to $n.", ch, obj, victim, TO_NOTVICT);
	act("$n makes a magical gesture and $p flys from your body to $s.", ch, obj, victim, TO_VICT);
	act("$p flys from $N to you.", ch, obj, victim, TO_CHAR);
	return;
}



/***************************************************************************
*	do_echo
***************************************************************************/
void do_echo(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	char buf[MSL];

	DENY_NPC(ch)

	if (argument[0] == '\0') {
		send_to_char("Global echo what?\n\r", ch);
		return;
	}

	for (d = descriptor_list; d; d = d->next) {
		if (d->connected == CON_PLAYING) {
			if (ch != NULL) {
				if (ch && get_trust(d->character) >= get_trust(ch)) {
					sprintf(buf, "%s global> ", ch->name);
					send_to_char(buf, d->character);
				}
			}
			send_to_char(argument, d->character);
			send_to_char("\n\r", d->character);
		}
	}

	return;
}


/***************************************************************************
*	do_recho
***************************************************************************/
void do_recho(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	char buf[MSL];

	DENY_NPC(ch)

	if (argument[0] == '\0') {
		send_to_char("Local echo what?\n\r", ch);
		return;
	}

	for (d = descriptor_list; d; d = d->next) {
		if (d->connected == CON_PLAYING
		    && d->character->in_room == ch->in_room) {
			if (ch != NULL
			    && get_trust(d->character) >= get_trust(ch)) {
				sprintf(buf, "%s local> ", ch->name);
				send_to_char(buf, d->character);
			}
			send_to_char(argument, d->character);
			send_to_char("\n\r", d->character);
		}
	}

	return;
}

/***************************************************************************
*	do_zecho
***************************************************************************/
void do_zecho(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	char buf[MSL];

	DENY_NPC(ch)

	if (argument[0] == '\0') {
		send_to_char("Zone echo what?\n\r", ch);
		return;
	}

	for (d = descriptor_list; d; d = d->next) {
		if (d->connected == CON_PLAYING
		    && d->character->in_room != NULL && ch->in_room != NULL
		    && d->character->in_room->area == ch->in_room->area) {
			if (ch != NULL
			    && get_trust(d->character) >= get_trust(ch)) {
				sprintf(buf, "%s zone> ", ch->name);
				send_to_char(buf, d->character);
			}
			send_to_char(argument, d->character);
			send_to_char("\n\r", d->character);
		}
	}
}

/***************************************************************************
*	do_pecho
***************************************************************************/
void do_pecho(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MIL];
	char buf[MSL];

	DENY_NPC(ch)

	argument = one_argument(argument, arg);
	if (argument[0] == '\0' || arg[0] == '\0') {
		send_to_char("Personal echo what?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("Target not found.\n\r", ch);
		return;
	}

	if (ch != NULL
	    && get_trust(victim) >= get_trust(ch)
	    && get_trust(ch) != MAX_LEVEL) {
		sprintf(buf, "%s personal> ", ch->name);
		send_to_char(buf, victim);
	}

	send_to_char(argument, victim);
	send_to_char("\n\r", victim);
	send_to_char("personal> ", ch);
	send_to_char(argument, ch);
	send_to_char("\n\r", ch);
}



/***************************************************************************
*	do_fry
***************************************************************************/
void do_fry(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char file[MIL];
	char arg[MIL];
	char vName[MIL];
	bool isImm;

	DENY_NPC(ch)

	if (CHECK_NON_OWNER(ch)) {
		sick_harvey_proctor(ch, hp_pissed_off, "Naughty, naughty, little boy!");
		send_to_char("You have been logged.\n\r", ch);
		return;
	} else {
		sick_harvey_proctor(ch, hp_agreeable, NULL);
	}

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Fry whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't playing.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("That's not a good idea.\n\r", ch);
		return;
	}

	/* don't forget to delete the god-file, too */
	/* added by Monrick, 1/2008                 */
	snprintf(vName, MIL, "%s", capitalize(victim->name));
	isImm = IS_IMMORTAL(victim);

	act("You summon a huge bolt of `&lightning`` which utterly destroys $N ", ch, NULL, victim, TO_CHAR);
	act("A huge bolt of `&lightning`` strikes $N, utterly `!destroying`` $M", ch, NULL, victim, TO_NOTVICT);
	act("You look up, just in time to see the `!flaming`` `&lightning`` bolt strike your head.\n\r C-ya!\n\r", ch, NULL, victim, TO_VICT);

	snprintf(file, MIL, "%s%s", PLAYER_DIR, vName);
	fry_char(victim, "");
	unlink(file);

	if (isImm) {
		sprintf(file, "%s%s", GOD_DIR, vName);
		unlink(file);
	}

	do_echo(ch, "You hear a rumble of ```5thunder``.\n\r");
	return;
}


/***************************************************************************
*	do_ffry
***************************************************************************/
void do_ffry(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MIL];

	DENY_NPC(ch)

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Fake fry whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't playing.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("That's not a good idea.\n\r", ch);
		return;
	}

	furniture_check(victim);
	act("You summon a huge bolt of `&lightning`` which utterly destroys $N ",
	    ch, NULL, victim, TO_CHAR);
	act("A huge bolt of `&lightning`` strikes $N, utterly `!destroying`` $s",
	    ch, NULL, victim, TO_NOTVICT);
	act("You look up, just in time to see the `!flaming`` `&lightning`` bolt strike your head.\n\r C-ya!\n\r",
	    ch, NULL, victim, TO_VICT);

	fry_char(victim, "");

	do_echo(ch, "You hear a rumble of ```5thunder``.\n\r");
	return;
}


/***************************************************************************
*	do_pardon
***************************************************************************/
void do_pardon(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg1[MIL];
	char arg2[MIL];

	DENY_NPC(ch)

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		send_to_char("Syntax: pardon <character> <killer|thief|idiot|target>.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "killer")) {
		if (IS_SET(victim->act, PLR_KILLER)) {
			REMOVE_BIT(victim->act, PLR_KILLER);
			send_to_char("Killer flag removed.\n\r", ch);
			send_to_char("You are no longer a KILLER.\n\r", victim);
		}
		return;
	}

	if (!str_cmp(arg2, "thief")) {
		if (IS_SET(victim->act, PLR_THIEF)) {
			REMOVE_BIT(victim->act, PLR_THIEF);
			send_to_char("Thief flag removed.\n\r", ch);
			send_to_char("You are no longer a THIEF.\n\r", victim);
		}
		return;
	}

	if (!str_cmp(arg2, "idiot")) {
		if (IS_SET(victim->act, PLR_IDIOT)) {
			REMOVE_BIT(victim->act, PLR_IDIOT);
			send_to_char("Idiot flag removed.\n\r", ch);
			send_to_char("You are no longer an `#IDIOT`7!\n\r", victim);
		}
		return;
	}

	if (!str_cmp(arg2, "target")) {
		if (IS_SET(victim->comm2, COMM2_TARGET)) {
			REMOVE_BIT(victim->comm2, COMM2_TARGET);
			send_to_char("Target status removed ..\n\r", ch);
			send_to_char("You are no longer a `Otarget`7.\n\r", victim);
		}
		return;
	}

	send_to_char("Syntax: pardon <character> <killer|thief|idiot|target>.\n\r", ch);
	return;
}




/***************************************************************************
*	do_idiot
***************************************************************************/
void do_idiot(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MSL];
	char buf[MSL];

	DENY_NPC(ch)

	one_argument(argument, arg);
	if (argument[0] == '\0') {
		send_to_char("Syntax: Idiot <character name>\n\r", ch);
		return;
	}

	victim = get_char_world(ch, arg);
	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("I don't think so ..\n\r", ch);
		return;
	}

	if (victim == NULL) {
		send_to_char("That player can't be found..\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("You can't do that to NPC's..\n\r", ch);
		return;
	}

	if (IS_SET(victim->act, PLR_IDIOT)) {
		send_to_char("They are nolonger an `#idiot`7..\n\r", ch);
		send_to_char("You don't feel like such an `#idiot`7 anymore..\n\r", victim);
		REMOVE_BIT(victim->act, PLR_IDIOT);
		return;
	}

	send_to_char("They are now an `#idiot`7!..\n\r", ch);
	send_to_char("You are an `#idiot`7!\n\r", victim);
	SET_BIT(victim->act, PLR_IDIOT);
	sprintf(buf, "%s", arg);
	do_unrestore(ch, buf);
	return;
}


/***************************************************************************
*       do_target
***************************************************************************/
void do_target(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MSL];

	DENY_NPC(ch)

	one_argument(argument, arg);
	if (argument[0] == '\0') {
		send_to_char("Syntax: Target <character name>\n\r", ch);
		return;
	}

	victim = get_char_world(ch, arg);
	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("I don't think so ..\n\r", ch);
		return;
	}

	if (victim == NULL) {
		send_to_char("That player can't be found..\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("You can't do that to NPC's..\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm2, COMM2_TARGET)) {
		send_to_char("They are nolonger a `#target`7..\n\r", ch);
		send_to_char("You don't feel like such an `#target`7 anymore..\n\r", victim);
		REMOVE_BIT(victim->comm2, COMM2_TARGET);
		return;
	}

	send_to_char("They are now a `#target`7!..\n\r", ch);
	SET_BIT(victim->comm2, COMM2_TARGET);
	return;
}

/***************************************************************************
*	do_norestore
***************************************************************************/
void do_norestore(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MSL];

	DENY_NPC(ch)

	one_argument(argument, arg);
	if (argument[0] == '\0') {
		send_to_char("Syntax: norestore <character name>\n\r", ch);
		return;
	}

	victim = get_char_world(ch, arg);
	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("I don't think so ..\n\r", ch);
		return;
	}

	if (victim == NULL) {
		send_to_char("That player can't be found..\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("You can't do that to NPCs.\n\r", ch);
		return;
	}

	if (IS_SET(victim->act, PLR_NORESTORE)) {
		send_to_char("They will now receive restores.\n\r", ch);
		send_to_char("You will now receive restores.\n\r", victim);
		REMOVE_BIT(victim->act, PLR_NORESTORE);
		return;
	}

	send_to_char("They no longer receive restores.\n\r", ch);
	send_to_char("You no longer receive restores.\n\r", victim);
	SET_BIT(victim->act, PLR_NORESTORE);
	return;
}






void do_transfer(CHAR_DATA *ch, char *argument)
{
	char arg1[MIL];
	char arg2[MIL];
	ROOM_INDEX_DATA *location;
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	DENY_NPC(ch)

	if (arg1[0] == '\0') {
		send_to_char("Transfer whom(and where)?\n\r", ch);
		return;
	}

	if (arg2[0] == '\0')
		location = ch->in_room;

	if (!str_cmp(arg1, "all")) {
		for (d = descriptor_list; d != NULL; d = d->next) {
			if (d->connected == CON_PLAYING
			    && d->character != ch
			    && d->character->in_room != NULL
			    && get_trust(d->character) < get_trust(ch)
			    && can_see(ch, d->character)) {
				char buf[MSL];

				sprintf(buf, "%s %s", d->character->name, arg2);
				do_transfer(ch, buf);
			}
		}
		return;
	}

/*
 * Thanks to Grodyn for the optional location parameter.
 */
	if (arg2[0] == '\0') {
		location = ch->in_room;
	} else {
		if ((location = find_location(ch, arg2)) == NULL) {
			send_to_char("No such location.\n\r", ch);
			return;
		}

		if (!is_room_owner(ch, location) && room_is_private(location)
		    && get_trust(ch) < LEVEL_IMMORTAL) {
			send_to_char("That room is private right now.\n\r", ch);
			return;
		}
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim->in_room == NULL) {
		send_to_char("They are in limbo.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("I don't think so ..\n\r", ch);
		return;
	}

	if (IS_SET(victim->act, PLR_BATTLE))
		REMOVE_BIT(victim->act, PLR_BATTLE);

	if (victim->fighting != NULL)
		stop_fighting(victim, TRUE);
	act("$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, location);
	act("$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM);
	if (ch != victim)
		act("$n has transferred you.", ch, NULL, victim, TO_VICT);
	do_look(victim, "auto");
	send_to_char("Ok.\n\r", ch);
}

void do_tarnsfer(CHAR_DATA *ch, char *argument)
{
	char arg1[MIL];
	char arg2[MIL];
	ROOM_INDEX_DATA *location;
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;

	DENY_NPC(ch)

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0') {
		send_to_char("Tarnsfer whom(and where)?\n\r", ch);
		return;
	}

	if (!str_cmp(arg1, "all")) {
		for (d = descriptor_list; d != NULL; d = d->next) {
			if (d->connected == CON_PLAYING
			    && d->character != ch
			    && d->character->in_room != NULL
			    && get_trust(d->character) < get_trust(ch)
			    && can_see(ch, d->character)) {
				char buf[MSL];

				sprintf(buf, "%s %s", d->character->name, arg2);
				do_tarnsfer(ch, buf);
			}
		}
		return;
	}

/*
 * Thanks to Grodyn for the optional location parameter.
 */
	if (arg2[0] == '\0') {
		location = ch->in_room;
	} else {
		if ((location = find_location(ch, arg2)) == NULL) {
			send_to_char("No such location.\n\r", ch);
			return;
		}

		if (!is_room_owner(ch, location) && room_is_private(location)
		    && get_trust(ch) < LEVEL_IMMORTAL) {
			send_to_char("That room is private right now.\n\r", ch);
			return;
		}
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim->in_room == NULL) {
		send_to_char("They are in limbo.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("I don't think so ..\n\r", ch);
		return;
	}

	if (victim->fighting != NULL)
		stop_fighting(victim, TRUE);
	act("$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, location);
	act("$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM);
	if (ch != victim)
		act("$n has tarnsferred you.", ch, NULL, victim, TO_VICT);
	do_look(victim, "auto");
	send_to_char("What's the matter, you catching `5G`Prapez `#syndrome`` or something?  Well, I\n\r", ch);
	send_to_char("can letcha slide THIS time.\n\r", ch);
	send_to_char("Ok.\n\r", ch);
}







void do_goto(CHAR_DATA *ch, char *argument)
{
	ROOM_INDEX_DATA *location;
	CHAR_DATA *rch;
	int count = 0;

	DENY_NPC(ch)

	if (argument[0] == '\0') {
		send_to_char("Goto where?\n\r", ch);
		return;
	}

	if ((location = find_location(ch, argument)) == NULL) {
		send_to_char("No such location.\n\r", ch);
		return;
	}

	count = 0;
	for (rch = location->people; rch != NULL; rch = rch->next_in_room)
		count++;

	if (!is_room_owner(ch, location) && room_is_private(location)
	    && (count > 1 || get_trust(ch) < LEVEL_IMMORTAL)) {
		send_to_char("That room is private right now.\n\r", ch);
		return;
	}

	if (ch->fighting != NULL)
		stop_fighting(ch, TRUE);

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
		if (get_trust(rch) >= ch->invis_level) {
			if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
				act("$t", ch, ch->pcdata->bamfout, rch, TO_VICT);
			else
				act("$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT);
		}
	}

	char_from_room(ch);
	char_to_room(ch, location);


	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
		if (get_trust(rch) >= ch->invis_level) {
			if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
				act("$t", ch, ch->pcdata->bamfin, rch, TO_VICT);
			else
				act("$n appears in a swirling mist.", ch, NULL, rch, TO_VICT);
		}
	}

	do_look(ch, "auto");
	return;
}

void do_violate(CHAR_DATA *ch, char *argument)
{
	ROOM_INDEX_DATA *location;
	CHAR_DATA *rch;

	DENY_NPC(ch)

	if (argument[0] == '\0') {
		send_to_char("Goto where?\n\r", ch);
		return;
	}

	if ((location = find_location(ch, argument)) == NULL) {
		send_to_char("No such location.\n\r", ch);
		return;
	}

	if (!room_is_private(location)) {
		send_to_char("That room isn't private, use goto.\n\r", ch);
		return;
	}

	if (ch->fighting != NULL)
		stop_fighting(ch, TRUE);

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
		if (get_trust(rch) >= ch->invis_level) {
			if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
				act("$t", ch, ch->pcdata->bamfout, rch, TO_VICT);
			else
				act("$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT);
		}
	}

	char_from_room(ch);
	char_to_room(ch, location);


	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
		if (get_trust(rch) >= ch->invis_level) {
			if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
				act("$t", ch, ch->pcdata->bamfin, rch, TO_VICT);
			else
				act("$n appears in a swirling mist.", ch, NULL, rch, TO_VICT);
		}
	}

	do_look(ch, "auto");
	return;
}

void do_reboo(CHAR_DATA *ch, char *argument)
{
	send_to_char("If you want to REBOOT, spell it out.\n\r", ch);
	return;
}



void do_reboot(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	DESCRIPTOR_DATA *d_next;
	char buf[MSL];
	extern bool merc_down;

	DENY_NPC(ch)

	if (ch) {
		sprintf(buf, "%s reaches for the little red reset button.", ch->name);
		do_echo(ch, buf);
	} else {
		do_echo(NULL, "Reboot by Acid-Fiend-1");
	}

	do_force(ch, "all save");
	if (ch)
		do_save(ch, "");
	merc_down = TRUE;

	for (d = descriptor_list; d != NULL; d = d_next) {
		d_next = d->next;
		close_socket(d);
	}

	return;
}



void do_shutdow(CHAR_DATA *ch, char *argument)
{
	send_to_char("If you want to SHUTDOWN, spell it out.\n\r", ch);
	return;
}

void do_shutdown(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	DESCRIPTOR_DATA *d_next;
	char buf[MSL];
	extern bool merc_down;

	DENY_NPC(ch)

	if (ch->invis_level < LEVEL_HERO) {
		sprintf(buf, "Shutdown by %s.\n\r", ch->name);
		do_echo(ch, buf);
	}

	sprintf(buf, "Shutdown by %s.", ch->name);
	append_file(ch, SHUTDOWN_FILE, buf);

	do_force(ch, "all save");
	do_save(ch, "");
	merc_down = TRUE;

	for (d = descriptor_list; d != NULL; d = d_next) {
		d_next = d->next;
		close_socket(d);
	}

	return;
}



void do_protect(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;

	DENY_NPC(ch)

	if (argument[0] == '\0') {
		send_to_char("Protect whom from snooping?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, argument)) == NULL) {
		send_to_char("You can't find them.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm, COMM_SNOOP_PROOF)) {
		act_new("$N is no longer snoop-proof.", ch, NULL, victim, TO_CHAR, POS_DEAD, FALSE);
		REMOVE_BIT(victim->comm, COMM_SNOOP_PROOF);
	} else {
		act_new("$N is now snoop-proof.", ch, NULL, victim, TO_CHAR, POS_DEAD, FALSE);
		send_to_char("You are now immune to snooping.\n\r", victim);
		SET_BIT(victim->comm, COMM_SNOOP_PROOF);
	}
}



void do_snoop(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;
	char buf[MSL];
	char arg[MIL];

	DENY_NPC(ch)

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Snoop whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim->desc == NULL) {
		send_to_char("No descriptor to snoop.\n\r", ch);
		return;
	}

	if (victim == ch) {
		send_to_char("Cancelling all snoops.\n\r", ch);
		wiznet("$N stops being such a snoop.",
		       ch, NULL, WIZ_SNOOPS,
		       WIZ_SECURE, get_trust(ch));

		for (d = descriptor_list; d != NULL; d = d->next)
			if (d->snoop_by == ch->desc)
				d->snoop_by = NULL;
		return;
	}

	if (victim->desc->snoop_by != NULL) {
		send_to_char("Busy already.\n\r", ch);
		return;
	}

	if (!is_room_owner(ch, victim->in_room) && ch->in_room != victim->in_room
	    && room_is_private(victim->in_room) && !IS_TRUSTED(ch, IMPLEMENTOR)) {
		send_to_char("That character is in a private room.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)
	    || IS_SET(victim->comm, COMM_SNOOP_PROOF)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (ch->desc != NULL) {
		for (d = ch->desc->snoop_by; d != NULL; d = d->snoop_by) {
			if (d->character == victim || d->original == victim) {
				send_to_char("No snoop loops.\n\r", ch);
				return;
			}
		}
	}

	victim->desc->snoop_by = ch->desc;
	sprintf(buf, "$N starts snooping on %s",
		(IS_NPC(ch) ? victim->short_descr : victim->name));
	wiznet(buf, ch, NULL, WIZ_SNOOPS, WIZ_SECURE, get_trust(ch));

	send_to_char("Ok.\n\r", ch);

	return;
}

/* SnoopList ..  November 1996  */
void do_snlist(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	char buf[MSL];

	DENY_NPC(ch)

	send_to_char("Currently snooped characters\n\r", ch);
	send_to_char("----------------------------\n\r", ch);

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		wch = (d->original != NULL) ? d->original : d->character;

		if (d->snoop_by != NULL) {
			sprintf(buf, "%-15s\n\r", wch->name);
			page_to_char(buf, ch);
		}
	}

	return;
}

void do_switch(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MIL];
	char buf[MSL];

	DENY_NPC(ch)

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Switch into whom?\n\r", ch);
		return;
	}

	if (ch->desc == NULL)
		return;


	if (ch->desc->original != NULL) {
		send_to_char("You are already switched.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim == ch) {
		send_to_char("Ok.\n\r", ch);
		return;
	}

	if (!IS_NPC(victim)) {
		send_to_char("You can only switch into mobiles.\n\r", ch);
		return;
	}

	if (!is_room_owner(ch, victim->in_room) && ch->in_room != victim->in_room
	    && room_is_private(victim->in_room) && !IS_TRUSTED(ch, IMPLEMENTOR)) {
		send_to_char("That character is in a private room.\n\r", ch);
		return;
	}

	if (victim->desc != NULL) {
		send_to_char("Character in use.\n\r", ch);
		return;
	}

	sprintf(buf, "$N switches into %s", victim->short_descr);
	wiznet(buf, ch, NULL, WIZ_SWITCHES, WIZ_SECURE, get_trust(ch));

	ch->desc->character = victim;
	ch->desc->original = ch;
	victim->desc = ch->desc;
	ch->desc = NULL;

/* change communications to match */
	if (ch->prompt != NULL)
		victim->prompt = str_dup(ch->prompt);

	victim->comm = ch->comm;
	victim->lines = ch->lines;

	send_to_char("Ok.\n\r", victim);
	return;
}



void do_return(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];

	if (ch->desc == NULL)
		return;

	if (ch->desc->original == NULL) {
		send_to_char("You aren't switched.\n\r", ch);
		return;
	}

	send_to_char("You return to your original body. Type replay to see any missed tells.\n\r", ch);
	if (ch->prompt != NULL) {
		free_string(ch->prompt);
		ch->prompt = NULL;
	}

	sprintf(buf, "$N returns from %s.", ch->short_descr);
	wiznet(buf, ch->desc->original, 0, WIZ_SWITCHES, WIZ_SECURE, get_trust(ch));

	ch->desc->character = ch->desc->original;
	ch->desc->original = NULL;
	ch->desc->character->desc = ch->desc;
	ch->desc = NULL;
	return;
}

/* trust levels for load and clone */
bool obj_check(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (IS_TRUSTED(ch, GOD)
	    || (IS_TRUSTED(ch, IMMORTAL) && obj->level <= 20 && obj->cost <= 1000)
	    || (IS_TRUSTED(ch, DEMI) && obj->level <= 10 && obj->cost <= 500)
	    || (IS_TRUSTED(ch, ANGEL) && obj->level <= 5 && obj->cost <= 250)
	    || (IS_TRUSTED(ch, AVATAR) && obj->level == 0 && obj->cost <= 100))
		return TRUE;
	else
		return FALSE;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *clone)
{
	OBJ_DATA *c_obj;
	OBJ_DATA *t_obj;

	for (c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content) {
		if (obj_check(ch, c_obj)) {
			t_obj = create_object(c_obj->obj_idx, 0);
			clone_object(c_obj, t_obj);
			obj_to_obj(t_obj, clone);
			recursive_clone(ch, c_obj, t_obj);
		}
	}
}


/* command that is similar to load */
void do_clone(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	char arg[MIL];
	char buf[MIL];
	char *rest;
	int count;
	int iter;

	count = 0;
	rest = one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Clone what?\n\r", ch);
		return;
	}

	if (arg[0] == '*') {
		send_to_char("You must put the number of items you want BEFORE the *.\n\r", ch);
		return;
	}

	if (!str_prefix(arg, "clone")) {
		send_to_char("Check your typing.  You only need to type clone ONCE.\n\r", ch);
		return;
	}

	if (!str_prefix(arg, "object")) {
		mob = NULL;

		count = mult_argument(rest, arg);
		obj = get_obj_here(ch, arg);
		if (obj == NULL) {
			send_to_char("You don't see that here.\n\r", ch);
			return;
		}
	} else if (!str_prefix(arg, "mobile") || !str_prefix(arg, "character")) {
		obj = NULL;

		count = mult_argument(rest, arg);
		mob = get_char_room(ch, arg);
		if (mob == NULL) {
			send_to_char("You don't see that here.\n\r", ch);
			return;
		}
	} else { /* find both */
		count = mult_argument(argument, arg);
		mob = get_char_room(ch, arg);
		obj = get_obj_here(ch, arg);

		if (mob == NULL && obj == NULL) {
			send_to_char("You don't see that here.\n\r", ch);
			return;
		}
	}

	if (count > 100) {
		send_to_char("You can clone a maximum of 100 objects or mobiles at once.\n\r", ch);
		return;
	}

	/* clone an object */
	if (obj != NULL) {
		if (!obj_check(ch, obj)) {
			send_to_char("Your powers are not great enough for such a task.\n\r", ch);
			return;
		} else {
			OBJ_DATA *clone = NULL;

			for (iter = 0; iter < count; iter++) {
				clone = create_object(obj->obj_idx, 0);
				clone_object(obj, clone);

				if (obj->carried_by != NULL)
					obj_to_char(clone, ch);
				else
					obj_to_room(clone, ch->in_room);
				recursive_clone(ch, obj, clone);
			}

			if (count > 1) {
				sprintf(buf, "$n has created [%d] $p.", count);
				act(buf, ch, clone, NULL, TO_ROOM);
				sprintf(buf, "You clone [%d] $p.", count);
				act(buf, ch, clone, NULL, TO_CHAR);
				sprintf(buf, "$N clones [%d] $p.", count);
				wiznet(buf, ch, clone, WIZ_LOAD, WIZ_SECURE, get_trust(ch));
			} else {
				act("$n has created $p.", ch, clone, NULL, TO_ROOM);
				act("You clone $p.", ch, clone, NULL, TO_CHAR);
				wiznet("$N clones $p.", ch, clone, WIZ_LOAD, WIZ_SECURE, get_trust(ch));
			}
			return;
		}
	} else if (mob != NULL) {
		if (!IS_NPC(mob)) {
			send_to_char("You can only clone mobiles.\n\r", ch);
			return;
		} else
		if ((mob->level > 20 && !IS_TRUSTED(ch, GOD))
		    || (mob->level > 10 && !IS_TRUSTED(ch, IMMORTAL))
		    || (mob->level > 5 && !IS_TRUSTED(ch, DEMI))
		    || (mob->level > 0 && !IS_TRUSTED(ch, ANGEL))
		    || !IS_TRUSTED(ch, AVATAR)) {
			send_to_char("Your powers are not great enough for such a task.\n\r", ch);
			return;
		} else {
			CHAR_DATA *clone = NULL;
			char buf[MSL];

			for (iter = 0; iter < count; iter++) {
				clone = create_mobile(mob->mob_idx);
				clone_mobile(mob, clone);

				for (obj = mob->carrying; obj != NULL; obj = obj->next_content) {
					if (obj_check(ch, obj)) {
						OBJ_DATA *new_obj = create_object(obj->obj_idx, 0);
						clone_object(obj, new_obj);
						recursive_clone(ch, obj, new_obj);
						obj_to_char(new_obj, clone);
						new_obj->wear_loc = obj->wear_loc;
					}
				}
				char_to_room(clone, ch->in_room);
			}

			if (clone != NULL) {
				if (count > 1) {
					sprintf(buf, "$n has created [%d] $N.", count);
					act(buf, ch, NULL, clone, TO_ROOM);
					sprintf(buf, "You clone [%d] $N.", count);
					act(buf, ch, NULL, clone, TO_CHAR);
					sprintf(buf, "$N clones [%d] %s.", count, clone->short_descr);
					wiznet(buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust(ch));
				} else {
					act("$n has created $N.", ch, NULL, clone, TO_ROOM);
					act("You clone $N.", ch, NULL, clone, TO_CHAR);
					sprintf(buf, "$N clones %s.", clone->short_descr);
					wiznet(buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust(ch));
				}
			}

			return;
		}
	}
}


void do_load(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];

	DENY_NPC(ch)

	argument = one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Syntax:\n\r", ch);
		send_to_char("  load mob <vnum>\n\r", ch);
		send_to_char("  load obj <vnum> <level>\n\r", ch);
		send_to_char("  load char <character>\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "mob")) {
		do_mload(ch, argument);
		return;
	}

	if (!str_cmp(arg, "obj")) {
		do_oload(ch, argument);
		return;
	}

	if (!str_cmp(arg, "char")) {
		char newarg[MIL];
		sprintf(newarg, "%s %s", "pload", argument);
		interpret(ch, newarg);
		return;
	}

	/* echo syntax */
	do_load(ch, "");
}


void do_mload(CHAR_DATA *ch, char *argument)
{
	MOB_INDEX_DATA *pMobIndex;
	CHAR_DATA *victim;
	char arg[MIL];
	char buf[MSL];

	one_argument(argument, arg);

	if (arg[0] == '\0' || !is_number(arg)) {
		send_to_char("Syntax: load mob <vnum>.\n\r", ch);
		return;
	}

	if ((pMobIndex = get_mob_index(atoi(arg))) == NULL) {
		send_to_char("No mob has that vnum.\n\r", ch);
		return;
	}

	victim = create_mobile(pMobIndex);
	char_to_room(victim, ch->in_room);

	act("$n has created $N!", ch, NULL, victim, TO_ROOM);
	snprintf(buf, MSL, "$N loads %s.", victim->short_descr);
	wiznet(buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust(ch));

	snprintf(buf, MSL, "mload: Loaded mob: %s\n\r", victim->short_descr);
	send_to_char(buf, ch);
	return;
}



void do_oload(CHAR_DATA *ch, char *argument)
{
	OBJ_INDEX_DATA *pObjIndex;
	OBJ_DATA *obj;
	char arg1[MIL];
	char arg2[MIL];
	char buf[MSL];
	int level;

	argument = one_argument(argument, arg1);
	one_argument(argument, arg2);

	if (arg1[0] == '\0' || !is_number(arg1)) {
		send_to_char("Syntax: load obj <vnum> <level>.\n\r", ch);
		return;
	}

	level = -1;
	if (arg2[0] != '\0') { /* load with a level */
		if (!is_number(arg2)) {
			send_to_char("Syntax: oload <vnum> <level>.\n\r", ch);
			return;
		}

		level = atoi(arg2);

		if (level < 0
		    || (get_trust(ch) != MAX_LEVEL
			&& (level > get_trust(ch)))) {
			send_to_char("Level must be be between 0 and your level.\n\r", ch);
			return;
		}
	}

	if ((pObjIndex = get_obj_index(atoi(arg1))) == NULL) {
		send_to_char("No object has that vnum.\n\r", ch);
		return;
	}

	obj = create_object(pObjIndex, level);

	if (CAN_WEAR(obj, ITEM_TAKE))
		obj_to_char(obj, ch);
	else
		obj_to_room(obj, ch->in_room);

	act("$n has created $p!", ch, obj, NULL, TO_ROOM);
	wiznet("$N loads $p.", ch, obj, WIZ_LOAD, WIZ_SECURE, get_trust(ch));

	snprintf(buf, MSL, "oload: Loaded object: %s\n\r", obj->short_descr);
	send_to_char(buf, ch);
	return;
}



void do_purge(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	char arg[MIL];
	char buf[100];

	one_argument(argument, arg);

	DENY_NPC(ch)

	if (arg[0] == '\0') {
		/* 'purge' */
		CHAR_DATA *vnext;
		OBJ_DATA *obj_next;

		for (victim = ch->in_room->people; victim != NULL; victim = vnext) {
			vnext = victim->next_in_room;
			if (IS_NPC(victim)
			    && !IS_SET(victim->act, ACT_NOPURGE)
			    && victim != ch)
				extract_char(victim, TRUE);
		}

		for (obj = ch->in_room->contents; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			if (!IS_OBJ_STAT(obj, ITEM_NOPURGE))
				extract_obj(obj);
		}

		act("$n purges the room!", ch, NULL, NULL, TO_ROOM);
		send_to_char("Ok.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (!IS_NPC(victim)) {
		if (ch == victim) {
			send_to_char("Ho ho ho.\n\r", ch);
			return;
		}

		if ((get_trust(ch) <= get_trust(victim)) && (victim->desc != NULL)) {
			send_to_char("Maybe that wasn't a good idea...\n\r", ch);
			sprintf(buf, "%s tried to purge you!\n\r", ch->name);                   send_to_char(buf, victim);
			return;
		}

		act("$n disintegrates $N.", ch, 0, victim, TO_NOTVICT);
		act("You turn $N to ash!", victim, NULL, NULL, TO_CHAR);
		act("$n disintegrates you!", ch, NULL, NULL, TO_VICT);

		if (victim->level > 1)
			save_char_obj(victim);

		d = victim->desc;
		extract_char(victim, TRUE);
		if (d != NULL)
			close_socket(d);

		return;
	}

	act("$n purges $N.", ch, NULL, victim, TO_NOTVICT);
	extract_char(victim, TRUE);
	return;
}




void do_advance(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg1[MIL];
	char arg2[MIL];
	char arg3[MIL];
	char buf[MSL];
	int level;

	DENY_NPC(ch)

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);

	if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2)) {
		send_to_char("Syntax: advance <char> <level>.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("That player is not here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}


	if (arg2[0] == '+') {
		if (victim->level >= LEVEL_HERO - 1)
			return;

		if (arg3[0] != '\0' && atoi(arg2) == 0)
			level = UMIN(victim->level + atoi(arg3), LEVEL_HERO - 1);
		else
			level = UMIN(victim->level + atoi(arg2), LEVEL_HERO - 1);
	} else if (arg2[0] == '-') {
		if (victim->level >= LEVEL_HERO || victim->level <= 1)
			return;

		if (arg3[0] != '\0' && atoi(arg2) == 0)
			level = UMAX(victim->level - atoi(arg3), 1);
		else
			level = UMAX(victim->level + atoi(arg2), 1);

	} else {
		level = atoi(arg2);
	}

	if (level < 1 || level > MAX_LEVEL) {
		send_to_char("Level must be 1 to 610.\n\r", ch);
		return;
	}

	if (level == 1 && CHECK_NON_OWNER(ch)) {
		sick_harvey_proctor(ch, hp_irritated, "You may only demote to level 2.");
		level = 2;
	}

	if (level == victim->level) {
		send_to_char("Level must be different than the characters current level.\n\r", ch);
		return;
	}


	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("Not at your trust level!\n\r", ch);
		return;
	}

	if (level > get_trust(ch)) {
		send_to_char("Limited to your trust level.\n\r", ch);
		return;
	}

	if (level <= victim->level) {
		printf_to_char(ch, "Lowering %s's level!\n\r", victim->name);
		send_to_char("**** OOOOHHHHHHHHHH  NNNNOOOO ****\n\r", victim);
	} else {
		printf_to_char(ch, "Raising %s's level!\n\r", victim->name);
		send_to_char("```O**** ```4OOOOHHHHHHHHHH  YYYYEEEESSS ```O****``\n\r", victim);
	}

	/* if we're lowering them back down to mortal status,    */
	/* then delete their god-file.  Added by Monrick, 1/2008 */
	if (IS_IMMORTAL(victim) && (level < LEVEL_IMMORTAL)) {
		sprintf(buf, "%s%s", GOD_DIR, victim->name);
		unlink(buf);
	}

	advance_level(victim, level - victim->level);

	victim->exp = exp_per_level(victim, victim->pcdata->points) * UMAX(1, victim->level);
	victim->trust = 0;

	save_char_obj(victim);
	return;
}



void do_trust(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg1[MIL];
	char arg2[MIL];
	int level;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	DENY_NPC(ch)
	if (CHECK_NON_OWNER(ch)) {
		sick_harvey_proctor(ch, hp_off_his_rocker, "NO! NO! NO!");
		return;
	}

	if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2)) {
		send_to_char("Syntax: trust <char> <level>.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("That player is not here.\n\r", ch);
		return;
	}

	level = atoi(arg2);
	if (level > MAX_LEVEL) {
		send_to_char("Level must be 0(reset) or 1 to 610.\n\r", ch);
		return;
	}

	if (level > get_trust(ch)) {
		send_to_char("Limited to your trust.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("Not at your trust level!\n\r", ch);
		return;
	}

	victim->trust = level;
	return;
}

void do_restore(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *vch;
	char arg1[MIL];
	char buf[MSL];

	one_argument(argument, arg1);

	if (!ch)
		return;

	if (IS_NPC(ch)) {
		send_to_char("Mobs can't use this command.\n\r", ch);
		return;
	}

	if ((arg1[0] == '\0'
	     || !str_cmp(arg1, "room"))
	    && (get_trust(ch) >= MAX_LEVEL - 1)) {
		for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
			restore_char(vch);

			if (ch->pcdata != NULL && ch->pcdata->rrestore_string[0] != '\0') {
				sprintf(buf, "%s", ch->pcdata->rrestore_string);
				send_to_char(buf, vch);
			} else {
				send_to_char("`1All`` of your aches have been `8soothed``.\n\r", vch);
			}
		}

		sprintf(buf, "$N restored room %ld.", ch->in_room->vnum);
		wiznet(buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, get_trust(ch));

		send_to_char("Room restored.\n\r", ch);
		return;
	}

	if (get_trust(ch) >= MAX_LEVEL - 1 && !str_cmp(arg1, "all")) {
		for (d = descriptor_list; d != NULL; d = d->next) {
			vch = d->character;

			if (vch == NULL || IS_NPC(vch))
				continue;

			restore_char(vch);
			save_char_obj(vch);

			if (vch->in_room != NULL) {
				if (ch->pcdata != NULL && ch->pcdata->grestore_string[0] != '\0') {
					sprintf(buf, "%s\n\r", ch->pcdata->grestore_string);
					send_to_char(buf, vch);
				} else {
					send_to_char("`1A `!huge`` beam of `&light`` crosses the sky above you, `4re`$st`6or`4in`$g`` you.\n\r", vch);
				}
			}
		}

		send_to_char("All active players restored and saved.\n\r", ch);
		return;
	}

	if ((vch = get_char_world(ch, arg1)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (get_trust(ch) < MAX_LEVEL - 1 && !IS_NPC(vch)) {
		send_to_char("You can only restore mobs ..\n\r", ch);
		return;
	}

	restore_char(vch);

	if (ch->pcdata != NULL && ch->pcdata->rrestore_string[0] != '\0')
		act("$t", ch, ch->pcdata->rrestore_string, vch, TO_VICT);
	else
		act("`1All`` of your aches have been `8soothed``.", ch, NULL, vch, TO_VICT);

	sprintf(buf, "$N restored %s", IS_NPC(vch) ? vch->short_descr : vch->name);
	wiznet(buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, get_trust(ch));

	send_to_char("Ok.\n\r", ch);
	return;
}

void do_unrestore(CHAR_DATA *ch, char *argument)
{
	char arg1[MIL];
	char buf[MSL];
	CHAR_DATA *victim;
	CHAR_DATA *vch;
	DESCRIPTOR_DATA *d;


	one_argument(argument, arg1);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (arg1[0] == '\0' || !str_cmp(arg1, "room")) {
		/* unrestore room */

		for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
			vch->hit = 1;
			vch->mana = 1;
			vch->move = 1;
			update_pos(vch);
			send_to_char("You suddenly feel `1very `8achy`` and `8sore``.\n\r", vch);
		}

		sprintf(buf, "$N un-restored room %ld.", ch->in_room->vnum);
		wiznet(buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, get_trust(ch));

		send_to_char("Room un-restored.\n\r", ch);
		return;
	}

	if (get_trust(ch) >= MAX_LEVEL - 2 && !str_cmp(arg1, "all")) {
		/* unrestore all */

		for (d = descriptor_list; d != NULL; d = d->next) {
			victim = d->character;

			if (victim == NULL || IS_NPC(victim))
				continue;

			victim->hit = 1;
			victim->mana = 1;
			victim->move = 1;
			update_pos(victim);
			save_char_obj(victim);
			if (victim->in_room != NULL)
				send_to_char("`1A `8dull`` beam of `&light`` crosses the sky above you, `!SMACKING`` you in the head!\n\r", victim);
		}
		send_to_char("All active players un-restored and saved.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	victim->hit = 1;
	victim->mana = 1;
	victim->move = 1;
	update_pos(victim);
	send_to_char("You suddenly feel `1very `8achy`` and `8sore``.\n\r", victim);
	sprintf(buf, "$N un-restored %s",
		IS_NPC(victim) ? victim->short_descr : victim->name);
	wiznet(buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, get_trust(ch));
	send_to_char("That oughtta teach the lil' bugger!\n\r", ch);
	return;
}


void do_freeze(CHAR_DATA *ch, char *argument)
{
	char arg[MIL], buf[MSL];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (arg[0] == '\0') {
		send_to_char("Freeze whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->act, PLR_FREEZE)) {
		REMOVE_BIT(victim->act, PLR_FREEZE);
		send_to_char("You can play again.\n\r", victim);
		send_to_char("FREEZE removed.\n\r", ch);
		sprintf(buf, "$N thaws %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->act, PLR_FREEZE);
		send_to_char("You can't do ANYthing!\n\r", victim);
		send_to_char("FREEZE set.\n\r", ch);
		sprintf(buf, "$N puts %s in the deep freeze.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	}

	save_char_obj(victim);

	return;
}

void do_affstrip(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char target[MIL];
	char affect[MSL];

	argument = one_argument(argument, target);

	DENY_NPC(ch);

	if (target[0] == '\0') {
		send_to_char("Affect strip whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, target)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (ch != victim && get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}


	argument = one_argument(argument, affect);

	if (affect[0] == '\0') {
		remove_all_affects(victim);
		send_to_char("You feel your being stripped of all magical affects.\n\r", victim);
	} else {
		SKILL *skill;

		if ((skill = skill_lookup(affect)) != NULL) {
			if (is_affected(victim, skill)) {
				affect_strip(victim, skill);
				printf_to_char(victim, "You have been stripped of the %s affect.\n\r",
					       skill->name);
			} else {
				send_to_char("They are not affected by that.\n\r", ch);
				return;
			}
		} else {
			send_to_char("That is not a valid affect.\n\r", ch);
			return;
		}
	}

	send_to_char("Done.\n\r", ch);
	save_char_obj(victim);

	return;
}



void do_log(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	DENY_NPC(ch)

	if (arg[0] == '\0') {
		send_to_char("Log whom?\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "all")) {
		if (log_all) {
			log_all = FALSE;
			send_to_char("Log ALL off.\n\r", ch);
		} else {
			log_all = TRUE;
			send_to_char("Log ALL on.\n\r", ch);
		}
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	/*
	 * No level check, gods can log anyone.
	 */
	if (IS_SET(victim->act, PLR_LOG)) {
		REMOVE_BIT(victim->act, PLR_LOG);
		send_to_char("LOG removed.\n\r", ch);
	} else {
		SET_BIT(victim->act, PLR_LOG);
		send_to_char("LOG set.\n\r", ch);
	}

	return;
}



void do_noemote(CHAR_DATA *ch, char *argument)
{
	char arg[MIL], buf[MSL];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (arg[0] == '\0') {
		send_to_char("Noemote whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}


	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm2, COMM2_NOEMOTE)) {
		REMOVE_BIT(victim->comm2, COMM2_NOEMOTE);
		send_to_char("You can emote again.\n\r", victim);
		send_to_char("NOEMOTE removed.\n\r", ch);
		sprintf(buf, "$N restores emotes to %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->comm2, COMM2_NOEMOTE);
		send_to_char("You can't emote!\n\r", victim);
		send_to_char("NOEMOTE set.\n\r", ch);
		sprintf(buf, "$N revokes %s's emotes.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	}

	return;
}

void do_notrivia(CHAR_DATA *ch, char *argument)
{
	char arg[MIL], buf[MSL];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (arg[0] == '\0') {
		send_to_char("No`!trivia`` whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}


	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm, COMM_TRIVIAOFF)) {
		REMOVE_BIT(victim->comm, COMM_TRIVIAOFF);
		send_to_char("You can use the `!trivia`` channel again.\n\r", victim);
		send_to_char("NOTRIVIA removed.\n\r", ch);
		sprintf(buf, "$N restores the trivia channel to %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->comm, COMM_TRIVIAOFF);
		send_to_char("You can't use the `!trivia`` channel!\n\r", victim);
		send_to_char("NOTRIVIA set.\n\r", ch);
		sprintf(buf, "$N revokes %s's trivia privileges.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	}

	return;
}
void do_nowish(CHAR_DATA *ch, char *argument)
{
	char arg[MIL], buf[MSL];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	DENY_NPC(ch);

	if (arg[0] == '\0') {
		send_to_char("No`Owish`` whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)
	    && victim != ch) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm, COMM_NOWISH)) {
		REMOVE_BIT(victim->comm, COMM_NOWISH);
		send_to_char("You can use the `Owish`` channel again.\n\r", victim);
		send_to_char("NOWISH removed.\n\r", ch);
		sprintf(buf, "$N restores the wish channel to %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->comm, COMM_NOWISH);
		send_to_char("You can't use the `Owish`` channel!\n\r", victim);
		send_to_char("NOWISH set.\n\r", ch);
		sprintf(buf, "$N revokes %s's wish privileges.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	}

	return;
}



void do_noshout(CHAR_DATA *ch, char *argument)
{
	char arg[MIL], buf[MSL];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (arg[0] == '\0') {
		send_to_char("Noshout whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm, COMM_NOSHOUT)) {
		REMOVE_BIT(victim->comm, COMM_NOSHOUT);
		send_to_char("You can shout again.\n\r", victim);
		send_to_char("NOSHOUT removed.\n\r", ch);
		sprintf(buf, "$N restores shouts to %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->comm, COMM_NOSHOUT);
		send_to_char("You can't shout!\n\r", victim);
		send_to_char("NOSHOUT set.\n\r", ch);
		sprintf(buf, "$N revokes %s's shouts.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	}

	return;
}



void do_notell(CHAR_DATA *ch, char *argument)
{
	char arg[MIL], buf[MSL];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (arg[0] == '\0') {
		send_to_char("Notell whom?", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm, COMM_NOTELL)) {
		REMOVE_BIT(victim->comm, COMM_NOTELL);
		send_to_char("You can tell again.\n\r", victim);
		send_to_char("NOTELL removed.\n\r", ch);
		sprintf(buf, "$N restores tells to %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->comm, COMM_NOTELL);
		send_to_char("You can't tell!\n\r", victim);
		send_to_char("NOTELL set.\n\r", ch);
		sprintf(buf, "$N revokes %s's tells.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	}

	return;
}



void do_peace(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *rch;

	DENY_NPC(ch)

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
		if (rch->fighting != NULL)
			stop_fighting(rch, TRUE);

		if (IS_NPC(rch) && IS_SET(rch->act, ACT_AGGRESSIVE))
			REMOVE_BIT(rch->act, ACT_AGGRESSIVE);
	}

	send_to_char("Ok.\n\r", ch);
	return;
}

void do_wizlock(CHAR_DATA *ch, char *argument)
{
	extern bool wizlock;

	wizlock = !wizlock;

	DENY_NPC(ch)

	if (wizlock) {
		wiznet("$N has wizlocked the game.", ch, NULL, 0, 0, 0);
		send_to_char("Game wizlocked.\n\r", ch);
	} else {
		wiznet("$N removes wizlock.", ch, NULL, 0, 0, 0);
		send_to_char("Game un-wizlocked.\n\r", ch);
	}

	return;
}

/* RT anti-newbie code */

void do_newlock(CHAR_DATA *ch, char *argument)
{
	extern bool newlock;

	newlock = !newlock;

	DENY_NPC(ch);

	if (newlock) {
		wiznet("$N locks out new characters.", ch, NULL, 0, 0, 0);
		send_to_char("New characters have been locked out.\n\r", ch);
	} else {
		wiznet("$N allows new characters back in.", ch, NULL, 0, 0, 0);
		send_to_char("Newlock removed.\n\r", ch);
	}

	return;
}



/***************************************************************************
*	do_speclist
***************************************************************************/
void do_mode(CHAR_DATA *ch, char *argument)
{
	extern bool wizlock;
	extern bool newlock;
	char arg[MIL];
	char arg2[MIL];

	argument = one_argument(argument, arg);
	argument = one_argument(argument, arg2);

	DENY_NPC(ch);

	if (arg[0] == '\0') {
		send_to_char("set which mode?\n\r", ch);
		send_to_char("	debug  status\n\r", ch);
		return;
	}

	if (!str_prefix(arg, "status")) {
		send_to_char("`PB`5ad `@T`2rip`7 mode status:\n\r", ch);

		send_to_char("`5Wizlock  `7 :                ", ch);
		if (wizlock)
			send_to_char("ON\n\r", ch);
		else
			send_to_char("`8OFF`7\n\r", ch);

		send_to_char("`#Newlock  `7 :                ", ch);
		if (newlock)
			send_to_char("ON\n\r", ch);
		else
			send_to_char("`8OFF`7\n\r", ch);

	}



	if (!str_prefix(arg, "wizlock")) {
		do_wizlock(ch, "");
		return;
	}

	if (!str_prefix(arg, "newlock")) {
		do_newlock(ch, "");
		return;
	}

	do_mode(ch, "");
}


/***************************************************************************
*	do_speclist
***************************************************************************/
void do_speclist(CHAR_DATA *ch, char *argument)
{
	do_help(ch, "speclist");
	return;
}

/***************************************************************************
*	do_slot
*
*	look up a spell slot
***************************************************************************/
void do_slot(CHAR_DATA *ch, char *argument)
{
	SKILL *skill;
	char arg[MIL];

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	smash_tilde(argument);
	argument = one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Syntax`8: ``slot <spell name>\n\r", ch);
		return;
	}

	if ((skill = skill_lookup(arg)) == NULL
	    || skill->spells == NULL) {
		send_to_char("That spell does not exist!\n\r", ch);
		return;
	}

	printf_to_char(ch, "The spell '%s' is slot number %d.\n\r", skill->name, skill->sn);
	return;
}





void do_sockets(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	BUFFER *buf;
	char *state;
	char arg[MIL];
	char sock[MSL];
	int count;
	int cmd;

	DENY_NPC(ch);

	count = 0;
	buf = new_buf();

	for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
		if (!str_prefix(cmd_table[cmd].name, "hidehost"))
			break;

	one_argument(argument, arg);
	send_to_char("`$Connection state            Socket  Name           IP Address", ch);
	send_to_char(get_trust(ch) >= cmd_table[cmd].level ? " `!(hidehost in red)``\n\r" : "\n\r", ch);
	send_to_char("``--------------------------+-------+--------------+------------------------------\n\r", ch);
	for (d = descriptor_list; d != NULL; d = d->next) {
		if (d->character != NULL && can_see(ch, d->character)
		    && (arg[0] == '\0' || is_name(arg, d->character->name)
			|| (d->original && is_name(arg, d->original->name)))) {
			count++;

			switch (d->connected) {
			case CON_PLAYING:
				state = "Connected and playing";
				break;
			case CON_GET_NAME:
				state = "Identifying remote user";
				break;
			case CON_GET_OLD_PASSWORD:
				state = "Entering password";
				break;
			case CON_CONFIRM_NEW_NAME:
				state = "Newbie: confirming name";
				break;
			case CON_GET_NEW_PASSWORD:
				state = "Entering new password";
				break;
			case CON_CONFIRM_NEW_PASSWORD:
				state = "Confirming new password";
				break;
			case CON_GET_NEW_RACE:
				state = "Newbie: Selecting race";
				break;
			case CON_GET_NEW_SEX:
				state = "Newbie: Selecting sex";
				break;
			case CON_GET_NEW_CLASS:
				state = "Newbie: Selecting class";
				break;
			case CON_GET_ALIGNMENT:
				state = "Newbie: Selecting align";
				break;
			case CON_DEFAULT_CHOICE:
				state = "Newbie: Getting default";
				break;
			case CON_GEN_GROUPS:
				state = "Newbie: Choosing groups";
				break;
			case CON_PICK_WEAPON:
				state = "Newbie: Choosing weapon";
				break;
			case CON_READ_IMOTD:
				state = "Reading IMOTD";
				break;
			case CON_READ_MOTD:
				state = "Reading MOTD";
				break;
			case CON_BREAK_CONNECT:
				state = "Breaking Connection";
				break;
			case CON_GET_ANSI:
				state = "Choosing ANSI [Y/n]";
				break;
			case CON_PKILL_CHOICE:
				state = "Newbie: Choosing PKILL";
				break;
			default:
				state = "-Unknown-";
				break;
			}


			if (IS_SET(d->character->comm2, COMM2_PRIV)) {
				if (get_trust(ch) >= get_trust(d->character))
					sprintf(sock, "`!%s``", d->host);
				else
					sprintf(sock, "-Unknown-");
			} else {
				sprintf(sock, "%s", d->host);
			}

			printf_buf(buf, "`@%-25.25s`` | `^%3d``   | %-12.12s | %s\n\r",
				   state,
				   d->descriptor,
				   d->original ? d->original->name :
				   d->character ? d->character->name : "(none)",
				   sock);
		}
	}

	if (count == 0) {
		send_to_char("No one by that name is connected.\n\r", ch);
		return;
	}

	add_buf(buf, "--------------------------+-------+--------------+------------------------------\n\r");
	printf_buf(buf, "%d user%s\n\r", count, count == 1 ? "" : "s");

	page_to_char(buf_string(buf), ch);
	free_buf(buf);
	return;
}


/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	char arg[MIL];
	char arg2[MIL];

	argument = one_argument(argument, arg);


	if (arg[0] == '\0' || argument[0] == '\0') {
		send_to_char("Force whom to do what?\n\r", ch);
		return;
	}

	one_argument(argument, arg2);

	if (!str_cmp(arg2, "suicide")) {
		send_to_char("Find a more creative way to kill ..\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "delete") || !str_cmp(arg2, "mob")) {
		send_to_char("That will NOT be done.\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "drag")) {
		send_to_char("Sorry, but that seems to crash the mud ..\n\r", ch);
		send_to_char("So `5DON'T DO IT`7! ..\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "pk")) {
		send_to_char("I'm sure they can make up their own mind ..\n\r", ch);
		return;
	}

	sprintf(buf, "$n forces you to '%s'.", argument);
	if (!str_cmp(arg, "all")) {
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		if (get_trust(ch) < MAX_LEVEL - 3) {
			send_to_char("Not at your level!\n\r", ch);
			return;
		}

		for (vch = char_list; vch != NULL; vch = vch_next) {
			vch_next = vch->next;

			if (!IS_NPC(vch) && get_trust(vch) < get_trust(ch)) {
				if (ch) {
					act(buf, ch, NULL, vch, TO_VICT);
				} else {
					sprintf(buf, "Acid-Fiend-1 forces you to '%s'.\n\r", argument);
					send_to_char(buf, vch);
				}

				if (!IS_NPC(vch) && vch->desc != NULL)
					substitute_alias(vch->desc, argument);
				else
					interpret(vch, argument);
			}
		}
	} else if (!str_cmp(arg, "players")) {
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		if (get_trust(ch) < MAX_LEVEL - 2) {
			send_to_char("Not at your level!\n\r", ch);
			return;
		}

		for (vch = char_list; vch != NULL; vch = vch_next) {
			vch_next = vch->next;

			if (!IS_NPC(vch) && get_trust(vch) < get_trust(ch)
			    && vch->level < LEVEL_HERO) {
				act(buf, ch, NULL, vch, TO_VICT);
				interpret(vch, argument);
			}
		}
	} else if (!str_cmp(arg, "gods")) {
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		if (get_trust(ch) < MAX_LEVEL - 2) {
			send_to_char("Not at your level!\n\r", ch);
			return;
		}

		for (vch = char_list; vch != NULL; vch = vch_next) {
			vch_next = vch->next;

			if (!IS_NPC(vch) && get_trust(vch) < get_trust(ch)
			    && vch->level >= LEVEL_HERO) {
				act(buf, ch, NULL, vch, TO_VICT);
				interpret(vch, argument);
			}
		}
	} else {
		CHAR_DATA *victim;

		if ((victim = get_char_world(ch, arg)) == NULL) {
			send_to_char("They aren't here.\n\r", ch);
			return;
		}

		if (victim == ch) {
			send_to_char("Aye aye, right away!\n\r", ch);
			return;
		}

		if (!is_room_owner(ch, victim->in_room)
		    && ch->in_room != victim->in_room
		    && room_is_private(victim->in_room) && !IS_TRUSTED(ch, IMPLEMENTOR)) {
			send_to_char("That character is in a private room.\n\r", ch);
			return;
		}

		if (get_trust(victim) >= get_trust(ch)) {
			send_to_char("Do it yourself!\n\r", ch);
			return;
		}

		if (!IS_NPC(victim) && get_trust(ch) < MAX_LEVEL - 3) {
			send_to_char("Not at your level!\n\r", ch);
			return;
		}

		act(buf, ch, NULL, victim, TO_VICT);
		if (!IS_NPC(victim) && victim->desc != NULL)
			substitute_alias(victim->desc, argument);
		else
			interpret(victim, argument);
	}

	if (ch)
		send_to_char("Ok.\n\r", ch);
	return;
}



/*
 * new routines by Dionysos.
 */
void do_winvis(CHAR_DATA *ch, char *argument)
{
	int level;
	char arg[MSL];

/* RT code for taking a level argument */
	(void)one_argument(argument, arg);

	DENY_NPC(ch)

	if (arg[0] == '\0') {
		/* take the default path */

		if (ch->invis_level) {
			ch->invis_level = 0;
			act("$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM);
			send_to_char("You slowly fade back into existence.\n\r", ch);
		} else {
			ch->invis_level = get_trust(ch);
			act("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM);
			send_to_char("You slowly vanish into thin air.\n\r", ch);
		}
	} else {
		/* do the level thing */
		level = atoi(arg);
		if (level < 2 || level > get_trust(ch)) {
			send_to_char("Invis level must be between 2 and your level.\n\r", ch);
			return;
		} else {
			ch->reply = NULL;
			ch->invis_level = level;
			act("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM);
			send_to_char("You slowly vanish into thin air.\n\r", ch);
		}
	}

	return;
}

void do_incognito(CHAR_DATA *ch, char *argument)
{
	int level;
	char arg[MSL];

	DENY_NPC(ch)

/* RT code for taking a level argument */
	one_argument(argument, arg);

	if (arg[0] == '\0') {
		/* take the default path */

		if (ch->incog_level) {
			ch->incog_level = 0;
			act("$n is no longer cloaked.", ch, NULL, NULL, TO_ROOM);
			send_to_char("You are no longer cloaked.\n\r", ch);
		} else {
			ch->incog_level = get_trust(ch);
			act("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM);
			send_to_char("You cloak your presence.\n\r", ch);
		}
	} else {
		/* do the level thing */
		level = atoi(arg);
		if (level < 2 || level > get_trust(ch)) {
			send_to_char("Incog level must be between 2 and your level.\n\r", ch);
			return;
		} else {
			ch->reply = NULL;
			ch->incog_level = level;
			act("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM);
			send_to_char("You cloak your presence.\n\r", ch);
		}
	}

	return;
}



void do_holylight(CHAR_DATA *ch, char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_HOLYLIGHT)) {
		REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
		send_to_char("Holy light mode off.\n\r", ch);
	} else {
		SET_BIT(ch->act, PLR_HOLYLIGHT);
		send_to_char("Holy light mode on.\n\r", ch);
	}

	return;
}

/* prefix command: it will put the string typed on each line typed */

void do_prefi(CHAR_DATA *ch, char *argument)
{
	send_to_char("You cannot abbreviate the prefix command.\r\n", ch);
	return;
}

void do_prefix(CHAR_DATA *ch, char *argument)
{
	char buf[MIL];

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (argument[0] == '\0') {
		if (ch->pcdata->prefix[0] == '\0') {
			send_to_char("You have no prefix to clear.\r\n", ch);
			return;
		}

		send_to_char("Prefix removed.\r\n", ch);
		free_string(ch->pcdata->prefix);
		ch->pcdata->prefix = str_dup("");
		return;
	}

	if (ch->pcdata->prefix[0] != '\0') {
		sprintf(buf, "Prefix changed to %s.\r\n", argument);
		free_string(ch->pcdata->prefix);
	} else {
		sprintf(buf, "Prefix set to %s.\r\n", argument);
	}

	ch->pcdata->prefix = str_dup(argument);
}

/*  Copyover - Original idea: Fusion of MUD++
 *  Adapted to Diku by Erwin S. Andreasen, <erwin@pip.dknet.dk>
 *  Changed into a ROM patch after seeing the 100th request for it :)
 */
void do_copyover(CHAR_DATA *ch, char *argument)
{
	FILE *fp, *cmdLog;
	DESCRIPTOR_DATA *d, *d_next;
	char buf [100], buf2[100];
	extern int port, control; /* db.c */

	fp = fopen(COPYOVER_FILE, "w");

	if (!fp) {
		send_to_char("Copyover file not writeable, aborted.\n\r", ch);
/*		logf ("Could not write to copyover file: %s", COPYOVER_FILE);*/
		perror("do_copyover:fopen");
		return;
	}

	/* Consider changing all saved areas here, if you use OLC */

	/* do_asave (NULL, ""); - autosave changed areas */


	sprintf(buf, "\n\r Preparing for a copyover....\n\r");

	/* For each playing descriptor, save its state */
	for (d = descriptor_list; d; d = d_next) {
		CHAR_DATA *och = CH(d);
		d_next = d->next; /* We delete from the list , so need to save this */

		if (!d->character || d->connected > CON_PLAYING) { /* drop those logging on */
			write_to_descriptor(d->descriptor, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r", 0);
			close_socket(d);  /* throw'em out */
		} else {
			fprintf(fp, "%d %s %s\n", d->descriptor, och->name, d->host);

			if (och->level == 1) {
				write_to_descriptor(d->descriptor, "Since you are level one, and level one characters do not save, you gain a free level!\n\r", 0);
				advance_level(och, 1);
			}
			do_stand(och, "");
			save_char_obj(och);
		}
	}

	fprintf(fp, "-1\n");
	fclose(fp);

	/* Close reserve and other always-open files and release other resources */

	fclose(fpReserve);

	/* Dalamar - Save our last commands file.. */

	if ((cmdLog = fopen(LAST_COMMANDS, "r")) == NULL) {
		log_string("Crash function: can't open last commands log..");
	} else {
		time_t rawtime;
		struct tm *timeinfo;
		char buf[128];
		char cmd[256];

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(buf, 128, "../log/command/lastCMDs-%m%d-%H%M.txt", timeinfo);
		sprintf(cmd, "mv ../log/command/lastCMDs.txt %s", buf);
		if (system(cmd) == -1) {
            log_string("System command failed: ");
            log_string(cmd);
        }
	}

	/* exec - descriptors are inherited */

	sprintf(buf, "%d", port);
	sprintf(buf2, "%d", control);
	execl(EXE_FILE, "Badtrip", buf, "copyover", buf2, (char *)NULL);

	/* Failed - sucessful exec will not return */

	perror("do_copyover: execl");
	send_to_char("Copyover FAILED!\n\r", ch);

	/* Here you might want to reopen fpReserve */
	fpReserve = fopen(NULL_FILE, "r");
}

/* Recover from a copyover - load players */
void copyover_recover()
{
	DESCRIPTOR_DATA *d;
	FILE *fp;
	char name [100];
	char host[MSL];
	int desc;
	bool fOld;

/*	logf ("Copyover recovery initiated");*/

	fp = fopen(COPYOVER_FILE, "r");

	if (!fp) { /* there are some descriptors open which will hang forever then ? */
		perror("copyover_recover:fopen");
/*		logf ("Copyover file not found. Exitting.\n\r");*/
		exit(1);
	}

	unlink(COPYOVER_FILE);  /* In case something crashes - doesn't prevent reading	*/

	for (;; ) {
        int scancount;
		scancount = fscanf(fp, "%d %s %s\n", &desc, name, host);
		if (scancount == EOF || desc == -1)
			break;

		/* Write something, and check if it goes error-free */
		if (!write_to_descriptor(desc, "", 0)) {
			close(desc);  /* nope */
			continue;
		}

		d = new_descriptor();
		d->descriptor = desc;

		d->host = str_dup(host);
		d->next = descriptor_list;
		descriptor_list = d;
		d->connected = CON_COPYOVER_RECOVER; /* -15, so close_socket frees the char */


		/* Now, find the pfile */

		fOld = load_char_obj(d, name);

		if (!fOld) { /* Player file not found?! */
			write_to_descriptor(desc, "\n\rSomehow, your character was lost in the copyover. Sorry.\n\r", 0);
			close_socket(d);
		} else { /* ok! */
/*			write_to_descriptor (desc, "\n\rCopyover recovery complete.\n\r",0);*/

			/* Just In Case */
			if (!d->character->in_room)
				d->character->in_room = get_room_index(ROOM_VNUM_TEMPLE);

			/* Insert in the char_list */
			d->character->next = char_list;
			char_list = d->character;

			send_to_char("\n\r`6o`&-`O-====`8---------------------------------------------------------`O===`&--`6o``\n\r", d->character);
			send_to_char("`2       ___    ___        __________________        ___    ___``\n\r", d->character);
			send_to_char("`2  ____/ _ \\__/ _ \\_____ (------------------) _____/ _ \\__/ _ \\____``\n\r", d->character);
			send_to_char("`2 (  _| / \\/  \\/ \\ |_   ) \\    `&Copyover`2    / (   _| / \\/  \\/ \\ |_  )``\n\r", d->character);
			send_to_char("`2  \\(  \\|  )  (  |/  ) (___)  __________  (___) (  \\|  )  (  |/  )/``\n\r", d->character);
			send_to_char("`2   '   '  \\`!''`2/  '  (_________)        (_________)  '  \\`!''`2/  '   '``\n\r", d->character);
			send_to_char("`2           ||                                          ||``\n\r", d->character);
			send_to_char("`6o`&-`O-====`8---------------------------------------------------------`O===`&--`6o``\n\r", d->character);
			char_to_room(d->character, d->character->in_room);
			do_look(d->character, "auto");
			act("$n materializes!", d->character, NULL, NULL, TO_ROOM);
			d->connected = CON_PLAYING;

			if (d->character->pet != NULL) {
				char_to_room(d->character->pet, d->character->in_room);
				act("$n materializes!.", d->character->pet, NULL, NULL, TO_ROOM);
			}
		}
	}
	fclose(fp);
}

/*
 * void do_enchant(CHAR_DATA * ch, char *argument)
 * {
 *  char item[MIL], affect[MIL], mod[MIL];
 *  OBJ_DATA *obj;
 *  AFFECT_DATA *aff;
 *
 *  argument = one_argument(argument, item);
 *  argument = one_argument(argument, affect);
 *  argument = one_argument(argument, mod);
 *
 *  if(ch)
 *  {
 *      if(IS_NPC(ch))
 *      {
 *          send_to_char("Mobs can't use this command.\n\r", ch);
 *          return;
 *      }
 *  }
 *
 *  if(item[0] == '\0' || affect[0] == '\0')
 *  {
 *      send_to_char("Enchant <item> <affect> [modifier]\n\r", ch);
 *      return;
 *  }
 *
 *  if((obj = get_obj_carry(ch, item)) == NULL)
 *  {
 *      send_to_char("You don't have that item.\n\r", ch);
 *      return;
 *  }
 *
 *  aff = malloc(sizeof(*aff));
 *  aff->level = ch->level;
 *  aff->duration = -1;
 *  aff->bitvector = 0;
 *  aff->type = aff->location = get_item_apply_val(affect);
 *  if(mod[0] != '\0')
 *      aff->modifier = atoi(mod);
 *  else
 *      aff->modifier = ch->level;
 *
 *
 *  affect_to_obj(obj, aff);
 * }
 */

bool check_parse_name(char *name);      /* comm.c */

void do_rename(CHAR_DATA *ch, char *argument)
{
	char old_name[MIL], new_name[MIL], strsave[MIL];

	CHAR_DATA *victim;
	FILE *file;

	argument = one_argument(argument, old_name);
	one_argument(argument, new_name);


/* Trivial checks */

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (!old_name[0]) {
		send_to_char("Rename who?\n\r", ch);
		return;
	}

	victim = get_char_world(ch, old_name);

	if (!victim) {
		send_to_char("There is no such a person online.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("You cannot use Rename on NPCs.\n\r", ch);
		return;
	}

/* allow rename self new_name,but otherwise only lower level */
	if ((victim != ch) && (get_trust(victim) >= get_trust(ch))) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (!new_name[0]) {
		send_to_char("Rename to what new name?\n\r", ch);
		return;
	}

	if (!check_parse_name(new_name)) {
		send_to_char("The new name is illegal.\n\r", ch);
		return;
	}

/* First, check if there is a player named that off-line */
	sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(new_name));

	fclose(fpReserve);              /* close the reserve file */
	file = fopen(strsave, "r");     /* attempt to to open pfile */

	if (file) {
		send_to_char("A player with that name already exists!\n\r", ch);
		fclose(file);
		fpReserve = fopen(NULL_FILE, "r");
		return;
	}

	fpReserve = fopen(NULL_FILE, "r");      /* reopen the extra file */

/* Check .gz file ! */
	sprintf(strsave, "%s%s.gz", PLAYER_DIR, capitalize(new_name));

	fclose(fpReserve);              /* close the reserve file */
	file = fopen(strsave, "r");     /* attempt to to open pfile */

	if (file) {
		send_to_char("A player with that name already exists in a compacted file.\n\r", ch);
		fclose(file);
		fpReserve = fopen(NULL_FILE, "r");
		return;
	}

	fpReserve = fopen(NULL_FILE, "r");      /* reopen the extra file */

	if (get_char_world(ch, new_name)) {
		send_to_char("A player with the name you specified already exists.\n\r", ch);
		return;
	}

/* Save the filename of the old name */

	sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(victim->name));

	unlink(strsave);

	/* check immortal stuff, added by Monrick 1/2008 */
	if (IS_IMMORTAL(victim)) {
		sprintf(strsave, "%s%s", GOD_DIR, capitalize(victim->name));
		unlink(strsave);
	}

	free_string(victim->name);
	victim->name = str_dup(capitalize(new_name));

	save_char_obj(victim);

/* unlink the old file
 * unlink(old_name);  */

	send_to_char("Character renamed.\n\r", ch);

	victim->position = POS_STANDING; /* I am laaazy */
	act("$n has renamed you to $N!", ch, NULL, victim, TO_VICT);
}


void do_faketell(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];

	argument = one_argument(argument, arg);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (arg[0] == '\0') {
		send_to_char("Syntax:(ftell)\n\r", ch);
		send_to_char("  ftell <personal> <victim> <message>\n\r", ch);
		send_to_char("  ftell <global> <victim> <message>\n\r", ch);
		send_to_char("  note: See 'ftell help' for help on \n\r", ch);
		send_to_char("        Faketell options ...\n\r", ch);
		return;
	}

	if (!str_prefix(arg, "help")) {
		send_to_char(" Faketell Help and stuff :\n\r", ch);
		send_to_char("  ftell   :  For general syntax of Faketell\n\r", ch);
		send_to_char("  global  :  Will send a Faketell to ALL\n\r", ch);
		send_to_char("            connected players .. The victim\n\r", ch);
		send_to_char("            will not be aware of the tell\n\r", ch);
		send_to_char("            that was sent out ..\n\r", ch);
		send_to_char("  personal:  Sends a faketell between two\n\r", ch);
		send_to_char("            victims, victim1 and victim2..\n\r", ch);
		send_to_char("            v1 is the sender, and v2 the\n\r", ch);
		send_to_char("            receiver ..\n\r", ch);
		send_to_char("\n\r", ch);
		return;
	}

	if (!str_prefix(arg, "personal")) {
		do_pftell(ch, argument);
		return;
	}

	if (!str_prefix(arg, "global")) {
		do_gftell(ch, argument);
		return;
	}

	do_faketell(ch, "");
}

void do_gftell(CHAR_DATA *ch, char *argument)
{
	char arg1[MIL];
	char arg2[MIL];
	char buf[MSL];
	CHAR_DATA *victim;
	DESCRIPTOR_DATA *d;

	argument = one_argument(argument, arg1);
	strcpy(arg2, argument);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (arg1[0] == '\0') {
		send_to_char("Syntax:(global)\n\r", ch);
		send_to_char("  ftell <victim> <message>\n\r", ch);
		return;
	}

	victim = get_char_world(ch, arg1);

	if ((victim == NULL) || (IS_NPC(victim))) {
		send_to_char("Your victim isn't currently in the world.\n\r", ch);
		return;
	}

	if (arg2[0] == '\0') {
		send_to_char("Faketell your victim what?\n", ch);
		return;
	}

	sprintf(buf, "\n\r`7Faketell(global):\n\r `@%s tells you '`t%s`@'``\n\r",
		capitalize(arg1), arg2);
	send_to_char(buf, ch);
	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		wch = (d->original != NULL) ? d->original : d->character;

		if (d->connected == CON_PLAYING &&
		    d->character != ch &&
		    !IS_SET(victim->comm, COMM_NOTELL) &&
		    !IS_SET(victim->comm, COMM_QUIET)) {
			if (wch != victim) {
				act_new("`@$n tells you '`t$t`@'`7", victim, argument, d->character, TO_VICT, POS_SLEEPING, FALSE);
				wch->reply = victim;
			}
		}
	}
	sprintf(buf, "%s ftell(global): `@%s tells you '`t%s`@'``\n\r",
		ch->name, arg1, arg2);
	wiznet(buf, ch, NULL, WIZ_SECURE, 0, 0);
	send_to_char("May your faketell cause lots of chaos =P ..\n\r\n\r", ch);
}

void do_pftell(CHAR_DATA *ch, char *argument)
{
	char arg1[MIL];
	char arg2[MIL];
	char arg3[MIL];
	char buf[MSL];
	CHAR_DATA *victim;
	CHAR_DATA *victim2;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	strcpy(arg3, argument);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (arg1[0] == '\0') {
		send_to_char("Syntax:(personal)\n\r", ch);
		send_to_char("  ftell <sender> <reciver> <message>\n\r", ch);
		return;
	}

	victim = get_char_world(ch, arg1);
	victim2 = get_char_world(ch, arg2);

	if ((victim == NULL) || (IS_NPC(victim))) {
		send_to_char("The sender of the faketell is not here..\n\r", ch);
		return;
	}

	if ((victim2 == NULL) || (IS_NPC(victim2))) {
		send_to_char("The recipiant of the faketell is not here..\n\r", ch);
		return;
	}


	if (arg3[0] == '\0') {
		send_to_char("Faketell your victim what?\n\r", ch);
		return;
	}

	sprintf(buf, "\n\r`7Faketell(personal to '%s'):\n\r `@%s tells you '`t%s`@'``\n\r",
		arg2, arg1, arg3);
	send_to_char(buf, ch);
	act_new("`@$n tells you '`1$t`@'`7", victim, argument, victim2, TO_VICT, POS_SLEEPING, FALSE);
	victim2->reply = victim;
	sprintf(buf, "%s ftell(personal to '%s'): `@%s tells you '`t%s`@'``\n\r",
		ch->name, arg2, arg1, arg3);
	wiznet(buf, ch, NULL, WIZ_SECURE, 0, 0);
	send_to_char("May your faketell cause lot's of chaos =P ..\n\r\n\r", ch);
}

/***************************************************************************
*	do_pnlist
***************************************************************************/
void do_pnlist(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;

	DENY_NPC(ch);

	send_to_char("Listing all connected penalized characters:\n\r", ch);
	send_to_char("+---------------------------------------------------------------------------+\n\r", ch);
	send_to_char("Name     | Frz| NoT| NoC| NoE| Log| Idt| Klr| Thi| SnP| Per| Pns| Dis\n\r", ch);
	send_to_char("+---------------------------------------------------------------------------+\n\r", ch);

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		wch = CH(d);

		if (wch == NULL || IS_NPC(wch))
			continue;

		if (!can_see(ch, wch))
			continue;

		printf_to_char(ch,
			       "%-13s%-9s%-9s%-9s%-9s%-9s%-9s%-9s%-9s%-9s%-9s%-9s%-9s\n\r",
			       wch->name,
			       IS_SET(wch->act, PLR_FREEZE) ? "`!X`7" : "`8-`7 ",
			       IS_SET(wch->comm, COMM_NOTELL) ? "`!X`7" : "`8-`7 ",
			       IS_SET(wch->comm, COMM_NOCHANNELS) ? "`!X`7" : "`8-`7 ",
			       IS_SET(wch->comm, COMM2_NOEMOTE) ? "`!X`7" : "`8-`7 ",
			       IS_SET(wch->act, PLR_LOG) ? "`!X`7" : "`8-`7 ",
			       IS_SET(wch->act, PLR_IDIOT) ? "`!X`7" : "`8-`7 ",
			       IS_SET(wch->act, PLR_KILLER) ? "`!X`7" : "`8-`7 ",
			       IS_SET(wch->act, PLR_THIEF) ? "`!X`7" : "`8-`7 ",
			       IS_SET(wch->comm, COMM_SNOOP_PROOF) ? "`!X`7" : "`8-`7 ",
			       IS_SET(wch->act, PLR_PERMIT) ? "`!X`7" : "`8-`7 ",
			       IS_SET(wch->act, PLR_PUNISHMENT) ? "`!X`7" : "`8-`7 ",
			       (wch->disabled != NULL) ? "`!X`7" : "`8-`7 ");
	}

	send_to_char("+---------------------------------------------------------------------------+\n\r", ch);
	return;
}


/***************************************************************************
*	do_pig
***************************************************************************/
void do_pig(CHAR_DATA *ch, char *argument)
{
	ROOM_INDEX_DATA *location;
	MOB_INDEX_DATA *pMobIndex;
	CHAR_DATA *mob;
	CHAR_DATA *victim;

	DENY_NPC(ch)

	if (argument[0] == '\0') {
		send_to_char("Who would you like to morph?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, argument)) == NULL) {
		send_to_char("They aren't here spunky.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Won't work on NPC's punk .. \n\r", ch);
		return;
	}

	if ((pMobIndex = get_mob_index(MOB_VNUM_PIG)) == NULL) {
		send_to_char("Couldn't find the PIGS vnum ..\n\r", ch);
		return;
	}

	if (victim->desc == NULL) {
		send_to_char("Already switched, like.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("uhmm, yeah ..\n\r", ch);
		return;
	}

	mob = create_mobile(pMobIndex);
	location = victim->in_room;
	char_from_room(victim);

	send_to_char("`#Squeeeeeeee!!! `3The ground looks a lot closer.\n``Oh `!NO!`` You're a `PPIG!``\n\n", victim);
	act("$n is suddenly turned into a pig!", victim, NULL, NULL, TO_ROOM);
	send_to_char("Ok.\n\r", ch);
	char_to_room(victim, get_room_index(ROOM_VNUM_LIMBO));
	save_char_obj(victim);
	char_to_room(mob, location);

	victim->desc->character = mob;
	victim->desc->original = victim;
	mob->desc = victim->desc;
	victim->desc = NULL;
	return;
}


/***************************************************************************
*	do_repop
***************************************************************************/
void do_repop(CHAR_DATA *ch, char *argument)
{
	AREA_DATA *pArea;

	DENY_NPC(ch)

	if (argument[0] == '\0') {
		reset_area(ch->in_room->area);
		send_to_char("Area repop!\n\r", ch);
	}


	if (!str_cmp(argument, "world")) {
		for (pArea = area_first; pArea != NULL; pArea = pArea->next) {
			reset_area(pArea);
			send_to_char("World Repop!", ch);
		}
	}

	return;
}

/***************************************************************************
*	do_immortal
***************************************************************************/
void do_immortal(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];

	DENY_NPC(ch)

	argument = one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Immortal Syntax:\n\r", ch);
		send_to_char("    kiss  <all> || <victim>\n\r", ch);
		send_to_char("    tag   <victim>\n\r", ch);
		return;
	}

	if (!str_prefix(arg, "kiss")) {
		do_immkiss(ch, argument);
		return;
	}

	if (!str_prefix(arg, "tag")) {
		do_immtag(ch, argument);
		return;
	}

	do_immortal(ch, "");
}


/***************************************************************************
*	do_immtag
***************************************************************************/
void do_immtag(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MIL];

	one_argument(argument, arg);
	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("You can't tag what's not there.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("You're not funny.\n\r", ch);
		return;
	}

	printf_to_char(ch, "%s is now IT.\n\r", victim->name);
	printf_to_char(victim, "%s's hand reaches down from the heavens and taps you. You're IT!\n\r", ch->name);

	SET_BIT(victim->act, PLR_IT);
	victim->pcdata->tag_ticks = 0;
	victim->pcdata->tagged_by = ch;
}


/***************************************************************************
*	do_immkiss
***************************************************************************/
void do_immkiss(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];
	char buf[MSL];
	CHAR_DATA *vch;
	DESCRIPTOR_DATA *d;

	one_argument(argument, arg);

	if ((!str_cmp(arg, "room")) || (arg[0] == '\0')) {
		send_to_char("You summon a `&ghostly`7 apparition of yourself", ch);

		for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
			sprintf(buf, "%s summons a `&ghostly`7 apparition of %sself.\n\r",
				PERS(ch, vch), ch->sex == 0 ? "it" : ch->sex == 1 ? "him" : "her");
			send_to_char(buf, vch);

			immkiss_char(ch, vch);
		}

		sprintf(buf, "$N kisses all in room %ld.", ch->in_room->vnum);
		wiznet(buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, get_trust(ch));
		send_to_char("Room kissed.\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "all")) {
		for (d = descriptor_list; d != NULL; d = d->next) {
			vch = d->character;

			if (vch == NULL || IS_NPC(vch) || vch == ch || IS_IMMORTAL(vch))
				continue;

			immkiss_char(ch, vch);
			save_char_obj(vch);
		}

		send_to_char("A `&ghostly`7 image of you floats across the land kissing all it passes.\n\r", ch);
		return;
	}

	if ((vch = get_char_world(ch, arg)) == NULL) {
		send_to_char("You can't kiss what's not there ..\n\r", ch);
		return;
	}

	immkiss_char(ch, vch);


	sprintf(buf, "$N kissed %s", IS_NPC(vch) ? vch->short_descr : vch->name);
	wiznet(buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, get_trust(ch));
	sprintf(buf, "You have immortally kissed %s", vch->name);
	send_to_char(buf, ch);
	return;
}


/***************************************************************************
*	do_omnistat
***************************************************************************/
void do_omnistat(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	BUFFER *output;
	char buf[MSL];
	int immmatch;
	int mortmatch;
	int hptemp;
	int manatemp = 0;

	DENY_NPC(ch)

	immmatch = 0;
	mortmatch = 0;
	output = new_buf();

	add_buf(output, " ----Immortals:----\n\r");
	add_buf(output, "Name          Level   Wiz   Incog   [Vnum]\n\r");

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		if (d->connected != CON_PLAYING)
			continue;

		wch = CH(d);

		if (!can_see(ch, wch) || wch->level < 601)
			continue;

		if (IS_IMMORTAL(wch)) {
			immmatch++;

			sprintf(buf, "%-14s %-3d    %-3d    %-3d    [%5ld]\n\r",
				wch->name,
				wch->level,
				wch->invis_level,
				wch->incog_level,
				wch->in_room->vnum);
			add_buf(output, buf);
		}
	}


	add_buf(output, " \n\r ----Mortals:----\n\r");
	add_buf(output, "Name           Race/Class   Position        Lev  %%hps  %%mana  [Vnum]\n\r");

	hptemp = 0;
	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;
		char const *class;

		if (d->connected != CON_PLAYING || !can_see(ch, d->character))
			continue;

		wch = CH(d);

		if (!can_see(ch, wch) || wch->level > ch->level || wch->level > 601)
			continue;

		mortmatch++;
		if ((wch->max_hit != wch->hit) && (wch->hit > 0))
			hptemp = (wch->hit * 100) / wch->max_hit;
		else if (wch->max_hit == wch->hit)
			hptemp = 100;
		else if (wch->hit < 0)
			hptemp = 0;

		if ((wch->max_mana != wch->mana) && (wch->mana > 0))
			manatemp = (wch->mana * 100) / wch->max_mana;
		else if (wch->max_mana == wch->mana)
			manatemp = 100;
		else if (wch->mana < 0)
			manatemp = 0;

		class = class_table[wch->class].who_name;
		sprintf(buf, "%-14s %5s/%3s    %-15s %-3d  %3d%%   %3d%%  [%5ld]\n\r",
			wch->name,
			wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name : "     ",
			class,
			capitalize(position_table[wch->position].name),
			wch->level,
			hptemp,
			manatemp,
			wch->in_room->vnum);
		add_buf(output, buf);
	}

	sprintf(buf, "\n\rImmortals found: `O%d`7\n\r", immmatch);
	add_buf(output, buf);

	sprintf(buf, "Mortals found: `O%d`7\n\r", mortmatch);
	add_buf(output, buf);

	page_to_char(buf_string(output), ch);

	free_buf(output);
	return;
}

void do_olevel(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	char arg[MIL];
	BUFFER *buffer;
	OBJ_INDEX_DATA *pObjIndex;
	long vnum, level;
	long nMatch;
	bool found;

	argument = one_argument(argument, arg);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (!is_number(arg)) {
		send_to_char("Syntax: olevel [level]\n\r", ch);
		return;
	}
	level = atoi(arg);
	buffer = new_buf();
	found = FALSE;
	nMatch = 0;

	for (vnum = 0; nMatch < top_obj_index; vnum++) {
		if ((pObjIndex = get_obj_index(vnum)) != NULL)
			nMatch++;
		if (level == pObjIndex->level) {
			found = TRUE;
			sprintf(buf, "`7[`O%5ld`7] `&%s`7\n\r", pObjIndex->vnum, pObjIndex->short_descr);
			add_buf(buffer, buf);
		}
	}
	if (!found)
		send_to_char("No objects by that level.\n\r", ch);
	else
		page_to_char(buf_string(buffer), ch);
	free_buf(buffer);
	return;
}

void do_mlevel(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];
	char arg[MIL];
	BUFFER *buffer;
	MOB_INDEX_DATA *pMobIndex;
	long vnum, level;
	long nMatch;
	bool found;

	argument = one_argument(argument, arg);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (!is_number(arg)) {
		send_to_char("Syntax: mlevel [level]\n\r", ch);
		return;
	}
	level = atoi(arg);
	buffer = new_buf();
	found = FALSE;
	nMatch = 0;

	for (vnum = 0; nMatch < top_mob_index; vnum++) {
		if ((pMobIndex = get_mob_index(vnum)) != NULL)
			nMatch++;
		if (level == pMobIndex->level) {
			found = TRUE;
			sprintf(buf, "`7[`O%5ld`7] `&%s`7\n\r", pMobIndex->vnum, pMobIndex->short_descr);
			add_buf(buffer, buf);
		}
	}
	if (!found)
		send_to_char("No mobiles found at that level ..\n\r", ch);
	else
		page_to_char(buf_string(buffer), ch);
	free_buf(buffer);
	return;
}

void do_setrestore(CHAR_DATA *ch, char *argument)
{
	char arg[MIL];

	argument = one_argument(argument, arg);

	smash_tilde(argument);

	if (IS_NPC(ch)) {
		send_to_char("You're not cool enough for restore strings ..\n\r", ch);
		return;
	}

	if (arg[0] == '\0') {
		send_to_char("Srestore syntax:\n\r", ch);
		send_to_char("  srestore room <text>  :  set restore string for rooms\n\r", ch);
		send_to_char("                           or individual players\n\r", ch);
		send_to_char("  srestore global <text>:  set restore string for global\n\r", ch);
		send_to_char("                           restore\n\r", ch);
		send_to_char("  srestore immkiss <text>: set immkiss string\n\r", ch);
		return;
	}

	if (!str_prefix(arg, "global")) {
		do_grestore(ch, argument);
		return;
	}

	if (!str_prefix(arg, "room")) {
		do_rrestore(ch, argument);
		return;
	}
	do_setrestore(ch, "");
}

void do_grestore(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];

	smash_tilde(argument);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (argument[0] == '\0') {
		if (ch->pcdata->grestore_string != NULL) {
			sprintf(buf, "Your `Oglobal`7 restore string is currently set to:\n%s\n\r",
				ch->pcdata->grestore_string);
			send_to_char(buf, ch);
			return;
		} else {
			send_to_char("You do not have a global restore string defined\n\r", ch);
		}
	}

	free_string(ch->pcdata->grestore_string);
	ch->pcdata->grestore_string = str_dup(argument);
	sprintf(buf, "Your `Oglobal`7 restore string is now set to:\n%s\n",
		ch->pcdata->grestore_string);
	send_to_char(buf, ch);
	return;
}

void do_rrestore(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];

	smash_tilde(argument);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (argument[0] == '\0') {
		if (ch->pcdata->rrestore_string != NULL) {
			sprintf(buf, "Your `Oroom`7 restore string is currently set to:\n%s\n\r",
				ch->pcdata->rrestore_string);
			send_to_char(buf, ch);
			return;
		} else {
			send_to_char("You do not have a restore string for rooms defined\n\r", ch);
		}
	}

	free_string(ch->pcdata->rrestore_string);
	ch->pcdata->rrestore_string = str_dup(argument);
	sprintf(buf, "Your `Oroom`7 restore string is now set to:\n%s\n",
		ch->pcdata->rrestore_string);
	send_to_char(buf, ch);
	return;
}

void do_immkissmsg(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];

	smash_tilde(argument);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}

	if (argument[0] == '\0') {
		if (ch->pcdata->immkiss_string != NULL) {
			sprintf(buf, "Your `Oimmkiss`7 string is currently set to:\n%s\n\r", ch->pcdata->immkiss_string);
			send_to_char(buf, ch);
			return;
		} else {
			send_to_char("You do not have a immkiss string defined\n\r", ch);
		}
	}

	free_string(ch->pcdata->immkiss_string);
	ch->pcdata->immkiss_string = str_dup(argument);
	sprintf(buf, "Your `Oimmkiss`7 string is now set to:\n%s\n", ch->pcdata->immkiss_string);
	send_to_char(buf, ch);
	return;
}

void do_review(CHAR_DATA *ch, char *argument)
{
	char buf[MSL];

	if (IS_NPC(ch)) {
		send_to_char("You have no identity ..\n\r", ch);
		return;
	}

	send_to_char("Here is a review of your settings.\n\r", ch);
	sprintf(buf, "Bamfin     : %s\n\r", ch->pcdata->bamfin);
	send_to_char(buf, ch);

	sprintf(buf, "Bamfout    : %s\n\r", ch->pcdata->bamfout);
	send_to_char(buf, ch);

	sprintf(buf, "G. Restore :%s\n\r", ch->pcdata->grestore_string);
	send_to_char(buf, ch);

	sprintf(buf, "R. Restore :%s\n\r", ch->pcdata->rrestore_string);
	send_to_char(buf, ch);

	sprintf(buf, "Immkiss    :%s\n\r", ch->pcdata->immkiss_string);
	send_to_char(buf, ch);
	sprintf(buf, "Title      :%s\n\r", ch->pcdata->title);
	send_to_char(buf, ch);

	sprintf(buf, "Long Desc  : %s\n\r", ch->long_descr);
	send_to_char(buf, ch);

	sprintf(buf, "Short Desc : %s\n\r", ch->short_descr);
	send_to_char(buf, ch);

	sprintf(buf, "Description: %s\n\r", ch->description);
	send_to_char(buf, ch);

	return;
}

/* Expand the name of a character into a string that identifies THAT
 * character within a room. E.g. the second 'guard' -> 2. guard
 */
const char *name_expand(CHAR_DATA *ch)
{
	int count = 1;
	CHAR_DATA *rch;
	char name[MIL];

	static char outbuf[MIL];

	if (!IS_NPC(ch))
		return ch->name;
	one_argument(ch->name, name);   /* copy the first word into name */

	if (!name[0]) {                 /* weird mob .. no keywords */
		strcpy(outbuf, "");
		return outbuf;
	}

	for (rch = ch->in_room->people; rch && (rch != ch); rch = rch->next_in_room)
		if (is_name(name, rch->name))
			count++;

	sprintf(outbuf, "%d.%s", count, name);
	return outbuf;
}


void do_punish(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MIL];


	DENY_NPC(ch);

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Punish whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("That is a good way to get spanked.\n\r", ch);
		return;
	}

	/*
	 * No level check, gods can punish ANYONE!  FEAR THAT!
	 */
	if (IS_SET(victim->act, PLR_PUNISHMENT)) {
		REMOVE_BIT(victim->act, PLR_PUNISHMENT);
		send_to_char("PUNISHMENT removed.\n\r", ch);
	} else {
		SET_BIT(victim->act, PLR_PUNISHMENT);
		send_to_char("PUNISHMENT set.\n\r", ch);
	}

	return;
}



void do_noquit(CHAR_DATA *ch, char *argument)
{
	char arg[MIL], buf[MSL];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (ch) {
		if (IS_NPC(ch)) {
			send_to_char("Mobs can't use this command.\n\r", ch);
			return;
		}
	}


	if (arg[0] == '\0') {
		send_to_char("Noquit whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm, COMM_NOQUIT)) {
		REMOVE_BIT(victim->comm, COMM_NOQUIT);
		send_to_char("You can quit again.\n\r", victim);
		send_to_char("NOQUIT removed.\n\r", ch);
		sprintf(buf, "$N restores quitting commands to %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->comm, COMM_NOQUIT);
		send_to_char("You can't quit!\n\r", victim);
		send_to_char("NOQUIT set.\n\r", ch);
		sprintf(buf, "$N revokes %s's quitting ability.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	}

	return;
}

/*
 * void do_disenchant(CHAR_DATA * ch, char *argument)
 * {
 *  char item[MIL];
 *  OBJ_DATA *obj;
 *  AFFECT_DATA *aff, *aff_next;
 *
 *
 *  argument = one_argument(argument, item);
 *
 *  if(ch)
 *  {
 *      if(IS_NPC(ch))
 *      {
 *          send_to_char("Mobs can't use this command.\n\r", ch);
 *          return;
 *      }
 *  }
 *
 *  if(item[0] == '\0')
 *  {
 *      send_to_char("Disenchant <item>\n\r", ch);
 *      return;
 *  }
 *
 *  if((obj = get_obj_carry(ch, item)) == NULL)
 *  {
 *      send_to_char("You don't have that item.\n\r", ch);
 *      return;
 *  }
 *
 *  for(aff = obj->affected; aff != NULL; aff = aff_next)
 *  {
 *      aff_next = aff->next;
 *      free_affect(aff);
 *  }
 *  obj->affected = NULL;
 *
 *  obj->extra_flags = 0;
 *  send_to_char("This object now has no enchantments upon it.\n\r", ch);
 *
 *  return;
 * }
 */
void do_rdesc(CHAR_DATA *ch, char *argument)
{
	ROOM_INDEX_DATA *location;
	char buf[MSL];

	DENY_NPC(ch);

	location = ch->in_room;
	if (location == NULL) {
		send_to_char("No such location.\n\r", ch);
		return;
	}

	if (!is_room_owner(ch, location) && ch->in_room != location
	    && room_is_private(location) && !IS_TRUSTED(ch, IMPLEMENTOR)) {
		send_to_char("That room is private right now.\n\r", ch);
		return;
	}

	if (argument[0] != '\0') {
		buf[0] = '\0';
		smash_tilde(argument);

		if (argument[0] == '-') {
			int len, buf_len;
			bool found = FALSE;

			if (location->description == NULL || location->description[0] == '\0') {
				send_to_char("No lines left to remove.\n\r", ch);
				return;
			}

			strcpy(buf, location->description);
			buf_len = (int)strlen(buf);
			for (len = buf_len; len > 0; len--) {
				if (buf[len] == '\r') {
					if (!found) {            /* back it up */
						if (len > 0)
							len--;
						found = TRUE;
					} else {
						buf[len + 1] = '\0';
						free_string(location->description);
						location->description = str_dup(buf);
						send_to_char("Room description is```8:``\n\r", ch);
						send_to_char(location->description ? location->description :
							     "(None).\n\r", ch);
						return;
					}
				}
			}

			buf[0] = '\0';
			free_string(location->description);
			location->description = str_dup(buf);
			send_to_char("Room description cleared.\n\r", ch);
			return;
		}

		if (argument[0] == '+') {
			if (location->description != NULL)
				strcat(buf, location->description);

			argument++;
			while (is_space(*argument))
				argument++;
		}

		if (strlen(buf) + strlen(argument) >= 1024) {
			send_to_char("Room description too long.\n\r", ch);
			return;
		}

		strcat(buf, argument);
		strcat(buf, "\n\r");
		free_string(location->description);
		location->description = str_dup(buf);
	}

	send_to_char("Room description is```8:``\n\r", ch);
	send_to_char(location->description ? location->description : "(None).\n\r", ch);
	return;
}


/***************************************************************************
*	do_addalias
*
*	add an alias to a characters alias list(with them knowing)
***************************************************************************/
void do_addalias(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *rch;
	char arg[MIL];
	int pos;

	argument = one_argument(argument, arg);
	if ((rch = get_char_world(ch, arg)) == NULL) {
		send_to_char("They are not here.\n\r", ch);
		return;
	}

	if (IS_NPC(rch))
		return;

	if (IS_SET(rch->act, PLR_LINKDEAD)) {
		send_to_char("They are linkdead, you cannot do that.\n\r", ch);
		return;
	}
	rch = rch->desc->original ? rch->desc->original : rch;

	argument = one_argument(argument, arg);
	smash_tilde(argument);

	if (arg[0] == '\0') {
		if (rch->pcdata->alias[0] == NULL) {
			send_to_char("They have no aliases defined.\n\r", ch);
			return;
		}

		send_to_char("Their current aliases are:\n\r", ch);
		for (pos = 0; pos < MAX_ALIAS; pos++) {
			if (rch->pcdata->alias[pos] == NULL
			    || rch->pcdata->alias_sub[pos] == NULL)
				break;

			printf_to_char(ch, "    %s:  %s\n\r", rch->pcdata->alias[pos],
				       rch->pcdata->alias_sub[pos]);
		}
		return;
	}

	if (!str_prefix("una", arg) || !str_cmp("alias", arg)) {
		send_to_char("Sorry, that word is reserved.\n\r", ch);
		return;
	}

	/* It seems the [ character crashes us .. *boggle* =)  */
	if (!str_prefix("[", arg)) {
		send_to_char("The [ Character is reserved..\n\r", ch);
		return;
	}

	if (argument[0] == '\0') {
		for (pos = 0; pos < MAX_ALIAS; pos++) {
			if (rch->pcdata->alias[pos] == NULL
			    || rch->pcdata->alias_sub[pos] == NULL)
				break;

			if (!str_cmp(arg, rch->pcdata->alias[pos])) {
				printf_to_char(ch, "%s aliases to '%s'.\n\r", rch->pcdata->alias[pos],
					       rch->pcdata->alias_sub[pos]);
				return;
			}
		}

		send_to_char("That alias is not defined.\n\r", ch);
		return;
	}

	if (!str_prefix(argument, "delete") || !str_prefix(argument, "prefix")) {
		send_to_char("That shall not be done!\n\r", ch);
		return;
	}

	if (add_alias(rch, arg, argument)) {
		printf_to_char(ch, "%s is now aliased to '%s'.\n\r", arg, argument);
	} else {
		send_to_char("Sorry, they have too many aliases.\n\r", ch);
		return;
	}
}





/***************************************************************************
*	utilities
***************************************************************************/
/***************************************************************************
*	fry_char
***************************************************************************/
void fry_char(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;

	DENY_NPC(ch)

	send_to_char("`!Get `Plost `#l```#o`#s```#e`#r``.\n\r", ch);
	sprintf(log_buf, "%s has quit for the last time.", ch->name);
	log_string(log_buf);

	save_char_obj(ch);
	d = ch->desc;

	char_from_room(ch);
	char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));
	extract_char(ch, TRUE);

	for (obj = get_room_index(ROOM_VNUM_LIMBO)->contents; obj != NULL; obj = obj_next) {
		obj_next = obj->next_content;
		/* obj_from_room(obj); <- wtf
		 * this is right, isnt it?!?
		 * didnt originally do the extract
		 * obj_from_room is probably a memory
		 * leak
		 */
		extract_obj(obj);
	}

	if (d != NULL)
		close_socket(d);

	return;
}


/***************************************************************************
 *	immkiss_char
 **************************************************************************/
void immkiss_char(CHAR_DATA *ch, CHAR_DATA *vch)
{
	SKILL *skill;
	char *spells[] = { "sanctuary", "shield", "bless",
			   "fly",	"haste",  "armor","giant strength","druid call", "frenzy", "" };
	int idx;

	if (IS_SET(ch->act, PLR_NORESTORE) || IS_SET(ch->act, PLR_IDIOT)) {
		send_to_char("You don't get restored.\n\r", ch);
		return;
	}

	restore_char(vch);
	if (ch->pcdata != NULL && ch->pcdata->immkiss_string[0] != '\0')
		printf_to_char(vch, "%s\n\r", ch->pcdata->immkiss_string);
	else
		printf_to_char(vch, "The image of %s kisses you lightly.\n\r", PERS(ch, vch));

/*	printf_to_char(vch, "The image of %s kisses you lightly.\n\r", PERS(ch, vch));*/

	for (idx = 0; spells[idx][0] != '\0'; idx++) {
		skill = skill_lookup(spells[idx]);
		cast_spell(ch, skill, ch->level * 2, vch, TARGET_CHAR, "");
	}
}

void do_listgods(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	BUFFER *gbuf;
	struct dirent *Dir;
	DIR *Directory;
	char FName[80];
	char buf[MSL];
	int nMatch, list1, list4;
	char list[MSL];
	char list2[MSL];
	char list3[MSL];

	nMatch = 0;

	gbuf = new_buf();
	d = new_descriptor();
	Directory = opendir(GOD_DIR);
	Dir = readdir(Directory);
	send_to_char("`^                      GOD LIST                      ``\n\r", ch);
	send_to_char(" `#Name          `@Lvl   `ORace         `&Class         `PTrust``\n\r", ch);
	send_to_char("`!=======================================================``\n\r", ch);
	while (Dir != NULL) {
		snprintf(FName, 80, "%s%s", GOD_DIR, Dir->d_name);
		{
			if (Dir->d_name[0] >= 'A' && Dir->d_name[0] <= 'Z') {
				nMatch++;
				load_char_obj(d, Dir->d_name);
				if (d->character->level <= ML) {
					sprintf(list, "%s", Dir->d_name);
					list1 = d->character->level;
					sprintf(list2, "%s", capitalize(race_table[d->character->race].name));
					sprintf(list3, "%s", capitalize(class_table[d->character->class].name));
					list4 = d->character->trust;


					sprintf(buf, "[`#%-12s``][`@%-2d`` ][`O%-10s`` ][`&%-12s`` ][`P%-3d`` ]\n\r", list, list1, list2, list3, list4);
					add_buf(gbuf, buf);
				}
			}
		}
		Dir = readdir(Directory);
	}
	closedir(Directory);
	page_to_char(buf_string(gbuf), ch);
	free_buf(gbuf);
	sprintf(buf, "\n\r`!Players found: `&%d``\n\r", nMatch);
	send_to_char(buf, ch);
	return;
}

/***************************************************************************
*	do_gobstopper
*
*	a "gobstopper" token makes it so a character never has to eat
*	or drink again.  these are currently redeemed manually by setting
*	the thirst and hunger properties to -151, but I am lazy
***************************************************************************/
void do_gobstopper(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *vch;

	DENY_NPC(ch);

	if (is_help(argument)) {
		send_to_char("Syntax: gobstopper <player name>\n\r", ch);
		return;
	}

	if ((vch = get_char_world(ch, argument)) == NULL) {
		send_to_char("There is no character by that name.\n\r", ch);
		return;
	}

	if (set_char_hunger(ch, vch, "-151") && set_char_thirst(ch, vch, "-151") && set_char_feed(ch, vch, "-151")) {
		send_to_char("You have been `OG`@o`1b`#s`Pt`@o`1p`#p`Pe`Or`@e`1d`#!`P!``.\n\r", vch);
		printf_to_char(ch, "You have `OG`@o`1b`#s`Pt`@o`1p`#p`Pe`Or`@e`1d`` %s!!.\n\r", vch->name);
	} else {
		printf_to_char(ch, "You failed to `OG`@o`1b`#s`Pt`@o`1p`#p`Pe`Or`` %s.\n\r",
			       (IS_NPC(vch)) ? vch->short_descr : vch->name);
	}
}




/***************************************************************************
*	testing stuff
***************************************************************************/

#if defined(__TEST_ENV)
#include "script.h"
/***************************************************************************
*	test_fn
*
*	function that can be changed for whatever code is necessary
***************************************************************************/
void test_fn()
{
	SCRIPT *script;
	SCRIPT_LINE *line;
	char *code = "if blah = blah\n\ror blah = blah2\n\rsay hi\n\relse\n\rsay bye\n\rendif\n\r";

	script = new_script();
	script->raw_code = str_dup(code);

	compile_script(script);

	if (script->error_line > 0) {
		printf("%d: %s\n", script->error_line, script->error_desc);
	} else {
		int ln;

		ln = 0;
		for (line = script->code; line != NULL; line = line->next)
			printf("%d: %s\n", ++ln, line->code);
	}
}

/***************************************************************************
*	do_immtest
*
*	code for doing testing of any recent changes
***************************************************************************/
void do_immtest(CHAR_DATA *ch, char *argument)
{
	test_fn();
}
#else
/***************************************************************************
*	do_immtest
*
*	code for doing testing of any recent changes
***************************************************************************/
void do_immtest(CHAR_DATA *ch, char *argument)
{
	send_to_char("This function is not available in the prodcution environment.\n\r", ch);
}
#endif

void do_auto_shutdown()
{
	FILE *fp, *cmdLog;
	DESCRIPTOR_DATA *d, *d_next;
	char buf [100], buf2[100];
	extern int port, control;

	fp = fopen(COPYOVER_FILE, "w");

	if (!fp) {
		for (d = descriptor_list; d != NULL; d = d_next) {
			if (d->character) {
				do_save(d->character, "");
				send_to_char("Ok I tried but we're crashing anyway sorry!\n\r", d->character);
			}

			d_next = d->next;
			close_socket(d);
		}

		exit(1);
	}

	if ((cmdLog = fopen(LAST_COMMANDS, "r")) == NULL) {
		log_string("Crash function: can't open last commands log..");
	} else {
		time_t rawtime;
		struct tm *timeinfo;
		char buf[128];
		char cmd[256];

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(buf, 128, "../log/command/lastCMDs-%m%d-%H%M.txt", timeinfo);
		sprintf(cmd, "mv ../log/command/lastCMDs.txt %s", buf);
		if (system(cmd) == -1) {
            log_string("System command failed: ");
            log_string(cmd);
        }
	}

	do_asave(NULL, "changed");

	sprintf(buf, "\n\rYour mud is crashing attempting a copyover now!\n\r");

	for (d = descriptor_list; d; d = d_next) {
		CHAR_DATA *och = CH(d);
		d_next = d->next; /* We delete from the list , so need to save this */

		if (!d->character || d->connected > CON_PLAYING) {
			write_to_descriptor(d->descriptor, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r", 0);
			close_socket(d); /* throw'em out */
		} else {
			fprintf(fp, "%d %s %s\n", d->descriptor, och->name, d->host);
			save_char_obj(och);
			write_to_descriptor(d->descriptor, buf, 0);
		}
	}

	fprintf(fp, "-1\n");
	fclose(fp);
	fclose(fpReserve);
	sprintf(buf, "%d", port);
	sprintf(buf2, "%d", control);
	execl(EXE_FILE, "rom", buf, "copyover", buf2, (char *)NULL);
	exit(1);
}

void do_mrelic(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *obj;
	int i = 1500;

	if (argument[0] == '\0') {
		send_to_char("Make a relic item of what?\n\r", ch);
		return;
	}

	if ((obj = get_obj_carry(ch, argument)) == NULL) {
		send_to_char("You do not have that item.\n\r", ch);
		return;
	}

	if (IS_OBJ_STAT2(obj, ITEM2_RELIC)) {
		REMOVE_BIT(obj->extra2_flags, ITEM2_RELIC);
		act("$p is no longer a relic item.", ch, obj, NULL, TO_CHAR);
	} else {
		SET_BIT(obj->extra2_flags, ITEM2_RELIC);
		if (obj->xp_tolevel <= 0)
			obj->xp_tolevel = i;
		act("$p is now a relic item.", ch, obj, NULL, TO_CHAR);
	}

	return;
}

/*************************************************
*  World Peace - stops all fighting in the game
*  Added by Monrick, 1/2008
*************************************************/
void do_wpeace(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *fch;

	for (fch = char_list; fch != NULL; fch = fch->next) {
		if (fch->desc == NULL || fch->desc->connected != CON_PLAYING)
			continue;

		if (fch->fighting != NULL) {
			stop_fighting(fch, TRUE);
			printf_to_char(fch, "%s has declared world peace.\n\r",
				       capitalize(ch->name));
		}
	}

	wiznet("$N has declared world peace.", ch, NULL, WIZ_SECURE, 0, get_trust(ch));
	return;
}

/*********************************************************
*  pLoad - original snippet by Gary McNickle (dharvest)
*          modified for BadTrip by Monrick, 1/2008
*********************************************************/
void do_ploa(CHAR_DATA *ch, char *argument)
{
	send_to_char("If you want to load a pfile, type out the whole command.\n\r", ch);
	return;
}

void do_pload(CHAR_DATA *ch, char *argument)
{
	DESCRIPTOR_DATA d;      /* need a blank descriptor for the pfile data */
	char vName[MIL];        /* victim Name */
	char buf[MSL];

	if (argument[0] == '\0') {
		send_to_char("Whose pfile do you want to load?\n\r", ch);
		return;
	}

	argument = one_argument(argument, vName);

	/* don't make duplicates */
	if (get_char_world(ch, vName) != NULL) {
		printf_to_char(ch, "%s is already connected to BadTrip.\n\r",
			       capitalize(vName));
		return;
	}

	if (load_char_obj(&d, vName)) {         /* found */
		d.character->desc = NULL;       /* so we know it's not a real player */
		d.character->next = char_list;
		d.character->was_in_room = d.character->in_room;
		char_list = d.character;
		d.connected = CON_PLAYING;
		SET_BIT(d.character->act, PLR_LINKDEAD);
		reset_char(d.character);

		if (d.character->in_room != NULL) {
			char_from_room(d.character);
			char_to_room(d.character, ch->in_room);
		}

		act("$n pulls $N from the nether regions of space and time.\n\r$N stares straight ahead, in a daze.",
		    ch, NULL, d.character, TO_ROOM);
		act("You pull $N from the nether regions of space and time.",
		    ch, NULL, d.character, TO_CHAR);
		sprintf(buf, "$N pLoad -> %s", capitalize(d.character->name));
		wiznet(buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust(ch));

		if (d.character->pet != NULL) {
			char_from_room(d.character->pet);
			char_to_room(d.character->pet, d.character->in_room);
			act("$n's loyal pet, $N, appears.",
			    d.character, NULL, d.character->pet, TO_ROOM);
		}
	} else { /* not found */
		printf_to_char(ch, "Cannot find a pfile for %s.  Check spelling and try again.\n\r", capitalize(vName));
	}
	return;
}

/*********************************************************
*  pUnload - original snippet by Gary McNickle (dharvest)
*          modified for BadTrip by Monrick, 1/2008
*********************************************************/
void do_punloa(CHAR_DATA *ch, char *argument)
{
	send_to_char("If you want to unload a pfile, type out the whole command.\n\r", ch);
	return;
}

void do_punload(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char vName[MIL];
	char buf[MSL];

	argument = one_argument(argument, vName);

	if ((victim = get_char_world(ch, vName)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim->desc != NULL) { /* they are legitimately logged on */
		send_to_char("Can't pUnload them - there's a person attached!\n\r", ch);
		return;
	}

	if (victim->was_in_room != NULL) {
		char_from_room(victim);
		char_to_room(victim, victim->was_in_room);
		victim->was_in_room = NULL;
		if (victim->pet != NULL) {
			char_from_room(victim->pet);
			char_to_room(victim->pet, victim->in_room);
		}
	}

	REMOVE_BIT(victim->act, PLR_LINKDEAD);
	save_char_obj(victim);
	do_quit(victim, "");

	act("$n has sent $N back into the nether regions of space and time.",
	    ch, NULL, victim, TO_ROOM);
	sprintf(buf, "$N pUnLoad -> %s", capitalize(victim->name));
	wiznet(buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust(ch));
	return;
}


void sick_harvey_proctor(CHAR_DATA *ch, enum e_harvey_proctor_is mood, const char *message)
{
	char buf[MSL];
	const char *censor_name = "Harvey Proctor";

	switch (mood) {
	case hp_pissed_off:
		snprintf(buf, MSL, "%s leaps out and canes your ass into submission shouting, '%s'\n\r", censor_name, message);
		break;

	case hp_irritated:
		snprintf(buf, MSL, "%s reaches out and slaps your legs saying, '%s'\n\r", censor_name, message);
		break;

	case hp_agreeable:
		snprintf(buf, MSL, "%s will allow this, but is ever watchful.\n\r", censor_name);
		break;

	case hp_off_his_rocker:
	default:
		snprintf(buf, MSL, "%s dick-slaps your face cackling, '%s'\n\r", censor_name, message);
		break;
	}

	send_to_char(buf, ch);
}
