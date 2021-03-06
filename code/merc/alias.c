#include <stdio.h>
#include <string.h>
#include "merc.h"



bool remove_alias(struct char_data * ch, char *alias);
bool add_alias(struct char_data * ch, char *alias, char *cmd);


/** expand an alias before calling into interpret */
void substitute_alias(struct descriptor_data *d, const char *argument)
{
    struct char_data *ch;
    char buf[MAX_STRING_LENGTH];
    char prefix[MAX_INPUT_LENGTH];
    char name[MAX_INPUT_LENGTH];
    const char *point;
    int alias;

    ch = CH(d);

    /* check for prefix */
    if (!IS_NPC(ch)) {
	if (ch->pcdata->prefix[0] != '\0' && str_prefix("prefix", argument)) {
	    if (strlen(ch->pcdata->prefix) + strlen(argument) > MAX_INPUT_LENGTH) {
		send_to_char("Line to long, prefix not processed.\r\n", ch);
	    } else {
		sprintf(prefix, "%s %s", ch->pcdata->prefix, argument);
		argument = prefix;
	    }
	}
    }

    if (IS_NPC(ch) || ch->pcdata->alias[0] == NULL || !str_prefix("alias", argument) || !str_prefix("una", argument) || !str_prefix("prefix", argument)) {
	interpret(d->character, argument);
	return;
    }

    strcpy(buf, argument);

    for (alias = 0; alias < MAX_ALIAS; alias++) {   /* go through the aliases */
	if (ch->pcdata->alias[alias] == NULL)
	    break;

	if (!str_prefix(ch->pcdata->alias[alias], argument)) {
	    point = one_argument(argument, name);
	    if (!strcmp(ch->pcdata->alias[alias], name)) {
		buf[0] = '\0';
		strcat(buf, ch->pcdata->alias_sub[alias]);
		if (point[0] != '\0') {
		    strcat(buf, " ");
		    strcat(buf, point);
		}
		break;
	    }
	    if (strlen(buf) > MAX_INPUT_LENGTH) {
		send_to_char("Alias substitution too long. Truncated.\r\n", ch);
		buf[MAX_INPUT_LENGTH - 1] = '\0';
	    }
	}
    }

    interpret(d->character, buf);
}


/***************************************************************************
 *	do_alia
 ***************************************************************************/
void do_alia(struct char_data *ch, const char *argument)
{
    send_to_char("I'm sorry, alias must be entered in full.\n\r", ch);
    return;
}


/***************************************************************************
 *	do_alias
 *
 *	alias a command
 ***************************************************************************/
void do_alias(struct char_data *ch, const char *argument)
{
    struct char_data *rch;
    char arg[MAX_INPUT_LENGTH];
    int pos;

    rch = CH(ch->desc);
    DENY_NPC(rch);

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
	if (rch->pcdata->alias[0] == NULL) {
	    send_to_char("You have no aliases defined.\n\r", ch);
	    return;
	}

	send_to_char("Your current aliases are:\n\r", ch);
	for (pos = 0; pos < MAX_ALIAS; pos++) {
	    if (rch->pcdata->alias[pos] == NULL || rch->pcdata->alias_sub[pos] == NULL)
		break;

	    printf_to_char(ch, "    %s:  %s\n\r", rch->pcdata->alias[pos], rch->pcdata->alias_sub[pos]);
	}
	return;
    }

    if (!str_prefix("una", arg) || !str_cmp("alias", arg)) {
	send_to_char("Sorry, that word is reserved.\n\r", ch);
	return;
    }

    if (strchr(arg, ' ') || strchr(arg, '"') || strchr(arg, '\'')) {
	send_to_char("The word to be aliased should not contain a space, a tick, or a double-quote.\n\r", ch);
	return;
    }

    /* It seems the [ character crashes us .. *boggle* =)  */
    if (!str_prefix("[", arg)) {
	send_to_char("The [ Character is reserved..\n\r", ch);
	return;
    }

    if (argument[0] == '\0') {
	for (pos = 0; pos < MAX_ALIAS; pos++) {
	    if (rch->pcdata->alias[pos] == NULL || rch->pcdata->alias_sub[pos] == NULL)
		break;

	    if (!str_cmp(arg, rch->pcdata->alias[pos])) {
		printf_to_char(ch, "%s aliases to '%s'.\n\r", rch->pcdata->alias[pos], rch->pcdata->alias_sub[pos]);
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

    {
	static char sanitized[MAX_INPUT_LENGTH];
	strncpy(sanitized, argument, MAX_INPUT_LENGTH);
	smash_tilde(arg);
	smash_tilde(sanitized);
	if (add_alias(rch, arg, sanitized)) {
	    printf_to_char(ch, "%s is now aliased to '%s'.\n\r", arg, sanitized);
	} else {
	    send_to_char("Sorry, you have reached the alias limit.\n\r", ch);
	    return;
	}
    }
}


/***************************************************************************
 *	do_unalias
 *
 *	unalias a command
 ***************************************************************************/
void do_unalias(struct char_data *ch, const char *argument)
{
    struct char_data *rch;
    char arg[MAX_INPUT_LENGTH];

    if (ch->desc == NULL)
	rch = ch;
    else
	rch = ch->desc->original ? ch->desc->original : ch;

    if (IS_NPC(rch))
	return;

    argument = one_argument(argument, arg);
    if (arg[0] == '\0') {
	send_to_char("Unalias what?\n\r", ch);
	return;
    }

    if (remove_alias(rch, arg))
	send_to_char("Alias removed.\n\r", ch);
    else
	send_to_char("No alias of that name to remove.\n\r", ch);
}


/***************************************************************************
 *	add_alias
 *
 *	add an alias to a characters pcdata
 ***************************************************************************/
bool add_alias(struct char_data *ch, char *alias, char *cmd)
{
    bool success = false;
    int pos;

    if (!IS_NPC(ch)) {
	for (pos = 0; pos < MAX_ALIAS; pos++) {
	    if (ch->pcdata->alias[pos] == NULL)
		break;

	    /* redefine an alias */
	    if (!str_cmp(alias, ch->pcdata->alias[pos])) {
		free_string(ch->pcdata->alias_sub[pos]);
		ch->pcdata->alias_sub[pos] = str_dup(cmd);
		success = true;
	    }
	}

	if (pos < MAX_ALIAS && !success) {
	    /* make a new alias */
	    ch->pcdata->alias[pos] = str_dup(alias);
	    ch->pcdata->alias_sub[pos] = str_dup(cmd);
	    success = true;
	}
    }

    return success;
}


/***************************************************************************
 *	remove_alias
 *
 *	remove an alias from a characters alias list
 ***************************************************************************/
bool remove_alias(struct char_data *ch, char *alias)
{
    bool success = false;
    int pos;

    if (!IS_NPC(ch)) {
	for (pos = 0; pos < MAX_ALIAS; pos++) {
	    if (ch->pcdata->alias[pos] == NULL)
		break;

	    if (success) {
		ch->pcdata->alias[pos - 1] = ch->pcdata->alias[pos];
		ch->pcdata->alias_sub[pos - 1] = ch->pcdata->alias_sub[pos];
		ch->pcdata->alias[pos] = NULL;
		ch->pcdata->alias_sub[pos] = NULL;
		continue;
	    }

	    if (!strcmp(alias, ch->pcdata->alias[pos])) {
		free_string(ch->pcdata->alias[pos]);
		free_string(ch->pcdata->alias_sub[pos]);

		ch->pcdata->alias[pos] = NULL;
		ch->pcdata->alias_sub[pos] = NULL;
		success = true;
	    }
	}
    }

    return success;
}
