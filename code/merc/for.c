/***************************************************************************
 *	BadTrip original code.
 *	1999-2000 by Joe Hall
 ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "help.h"

/** exports */
bool expand_cmd(struct char_data * vch, const char *arg, char *buf, char find);


/** locals */
typedef void FOR_CMD (struct char_data *ch, char *name, const char *argument);

static void for_all(struct char_data * ch, char *name, const char *argument);
static void for_gods(struct char_data * ch, char *name, const char *argument);
static void for_morts(struct char_data * ch, char *name, const char *argument);
static void for_room(struct char_data * ch, char *name, const char *argument);
static void for_name(struct char_data * ch, char *name, const char *argument);
static void for_count(struct char_data * ch, char *name, const char *argument);
static bool valid_cmd(struct char_data * vcn, const char *cmd);
static const char *get_name(struct char_data * vch);

static const struct for_cmds {
    char *		cmd;
    FOR_CMD *	fn;
} fcmd_table [] = {
    { "all",     for_all	},
    { "gods",    for_gods	},
    { "mortals", for_morts	},
    { "room",    for_room	},
    { NULL,	     NULL	}
};


#define MAX_TARGETS             100
#define FOR_WAIT                36

static const char *invalid_cmds[] =
{
    "quit", "for", "fry", "ffry", "string", "disconnect", "purge", NULL
};

bool valid_cmd(struct char_data *vch, const char *cmd)
{
    const CMD *cmd_base;
    bool success = true;
    int iter;

    if ((cmd_base = cmd_lookup(vch, cmd)) != NULL) {
        for (iter = 0; invalid_cmds[iter] != NULL; iter++) {
            if (!str_cmp(invalid_cmds[iter], cmd_base->name)) {
                success = false;
                break;
            }
        }
    }

    return success;
}

void do_for(struct char_data *ch, const char *argument)
{
    char name[MAX_INPUT_LENGTH];
    int iter;

    argument = one_argument(argument, name);

    DENY_NPC(ch);

    if (!name[0] || !argument[0]) {
        show_help(ch->desc, "immortal_for", NULL);
        return;
    }

    if (!valid_cmd(ch, argument)) {
        send_to_char("The command you have entered is not safe for the `2FOR`` command.\n\r", ch);
        return;
    }

    for (iter = 0; fcmd_table[iter].cmd != NULL; iter++) {
        if (!str_prefix(name, fcmd_table[iter].cmd)) {
            (*fcmd_table[iter].fn)(ch, name, argument);

            if (ch->level < MAX_LEVEL)
                WAIT_STATE(ch, FOR_WAIT);
            return;
        }
    }


    if (is_number(name)) {
        for_count(ch, name, argument);
    } else if (strlen(name) > 1) {
        for_name(ch, name, argument);
    } else {
        show_help(ch->desc, "for", NULL);
        return;
    }

    if (ch->level < MAX_LEVEL) {
        WAIT_STATE(ch, FOR_WAIT);
    }
}



/***************************************************************************
 *	for_all
 *
 *	for all players -- gods and mortals
 ***************************************************************************/
static void for_all(struct char_data *ch, char *name, const char *argument)
{
    for_gods(ch, name, argument);
    for_morts(ch, name, argument);
}


/***************************************************************************
 *	for_gods
 *
 *	for all imms
 ***************************************************************************/
static void for_gods(struct char_data *ch, char *name, const char *argument)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true, .skip_character = ch };
    struct descriptor_data *d;
    struct descriptor_data *dpending;
    struct roomtemplate *origin;
    struct area_data *area;
    char cmd[MAX_STRING_LENGTH];
    int count = 0;
    char check[MAX_INPUT_LENGTH];


    origin = ch->in_room;
    area = NULL;
    one_argument(argument, check);
    if (!str_prefix(check, "area")) {
        argument = one_argument(argument, check);
        area = ch->in_room->area;
    }

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
        dpending = descriptor_iterator(d, &playing_filter);

        if (IS_IMMORTAL(d->character)
            && d->character->in_room != NULL
            && !room_is_private(d->character->in_room)
            && (area == NULL || d->character->in_room->area == area)) {
            if (++count > MAX_TARGETS) {
                send_to_char("Maximum number of targets exceeded.\n\r", ch);
                break;
            }

            char_from_room(ch);
            char_to_room(ch, d->character->in_room);

            if (expand_cmd(d->character, argument, cmd, '#'))
                interpret(ch, cmd);
        }
    }

    char_from_room(ch);
    char_to_room(ch, origin);
}


/**
 * for_morts
 */
static void for_morts(struct char_data *ch, char *name, const char *argument)
{
    struct descriptor_iterator_filter playing_filter = { .must_playing = true, .skip_character = ch };
    struct descriptor_data *d;
    struct descriptor_data *dpending;
    struct roomtemplate *origin;
    struct area_data *area;
    char cmd[MAX_STRING_LENGTH];
    int count = 0;
    char check[MAX_INPUT_LENGTH];

    origin = ch->in_room;
    area = NULL;
    one_argument(argument, check);
    if (!str_prefix(check, "area")) {
        argument = one_argument(argument, check);
        area = ch->in_room->area;
    }

    dpending = descriptor_iterator_start(&playing_filter);
    while ((d = dpending) != NULL) {
        dpending = descriptor_iterator(d, &playing_filter);

        if (!IS_IMMORTAL(d->character)
            && d->character->in_room != NULL
            && !room_is_private(d->character->in_room)
            && (area == NULL || d->character->in_room->area == area)) {
            if (++count > MAX_TARGETS) {
                send_to_char("Maximum number of targets exceeded.\n\r", ch);
                break;
            }

            char_from_room(ch);
            char_to_room(ch, d->character->in_room);

            if (expand_cmd(d->character, argument, cmd, '#'))
                interpret(ch, cmd);
        }
    }

    char_from_room(ch);
    char_to_room(ch, origin);
}


/***************************************************************************
 *	for_room
 *
 *	for each person in the room - affect mobiles
 ***************************************************************************/
static void for_room(struct char_data *ch, char *name, const char *argument)
{
    struct char_data *vch;
    struct char_data *vch_next;
    char cmd[MAX_STRING_LENGTH];
    int count = 0;

    for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
        vch_next = vch->next_in_room;;

        if (vch != ch) {
            if (++count > MAX_TARGETS) {
                send_to_char("Maximum number of targets exceeded.\n\r", ch);
                break;
            }

            if (expand_cmd(vch, argument, cmd, '#'))
                interpret(ch, cmd);
        }
    }
}


/***************************************************************************
 *	for_name
 *
 *	for each character with a given name -- PC and NPCs
 ***************************************************************************/
static void for_name(struct char_data *ch, char *name, const char *argument)
{
    struct roomtemplate *origin;
    struct char_data *vch;
    struct char_data *vch_next;
    struct area_data *area;
    char cmd[MAX_STRING_LENGTH];
    int count = 0;
    char check[MAX_INPUT_LENGTH];

    origin = ch->in_room;
    area = NULL;
    one_argument(argument, check);
    if (!str_prefix(check, "area")) {
        argument = one_argument(argument, check);
        area = ch->in_room->area;
    }

    for (vch = char_list; vch != NULL; vch = vch_next) {
        vch_next = vch->next;

        if (is_name(name, vch->name)
            && vch != ch
            && !room_is_private(vch->in_room)
            && (area == NULL || vch->in_room->area == area)) {
            if (++count > MAX_TARGETS) {
                send_to_char("Maximum number of targets exceeded.\n\r", ch);
                break;
            }

            char_from_room(ch);
            char_to_room(ch, vch->in_room);

            if (expand_cmd(vch, argument, cmd, '#'))
                interpret(ch, cmd);
        }
    }

    char_from_room(ch);
    char_to_room(ch, origin);
}




/***************************************************************************
 *	for_count
 *
 *	do a command parse_int(name) times
 ***************************************************************************/
static void for_count(struct char_data *ch, char *name, const char *argument)
{
    int count = 0;
    int number;
    int iter;

    if (is_number(name)) {
        number = parse_int(name);
        if (number > 201) {
            send_to_char("There is a maximum of 200 targets when using the for command.\n\r", ch);
            return;
        }
        if (number > 0) {
            for (iter = 0; iter < number; iter++) {
                if (++count > MAX_TARGETS) {
                    send_to_char("Maximum number of targets exceeded.\n\r", ch);
                    break;
                }

                interpret(ch, argument);
            }
        }
    }
}

/***************************************************************************
 *	expand_cmd
 *
 *	expands any special characters(defined by "find") in a
 *	command to a valid target
 ***************************************************************************/
bool expand_cmd(struct char_data *vch, const char *arg, char *buf, char find)
{
    const char *name = get_name(vch);
    const char *orig = arg;
    char *dest = buf;
    int len = 0;

    *dest = '\0';
    while (*orig != '\0') {
        if (++len >= MAX_STRING_LENGTH)
            break;

        if (*orig == find) {
            const char *tmp = name;
            while (*tmp != '\0')
                *dest++ = *tmp++;
            orig++;
        } else {
            *dest++ = *orig++;
        }
    }

    *dest = '\0';
    return len < MAX_STRING_LENGTH;
}


/***************************************************************************
 *	get_name
 *
 *	gets the proper name(with #.xxx notation) for a character
 ***************************************************************************/
static const char *get_name(struct char_data *vch)
{
    struct char_data *rch;
    char name[MAX_INPUT_LENGTH];
    static char outbuf[MAX_INPUT_LENGTH];


    if (!IS_NPC(vch))
        return vch->name;

    one_argument(vch->name, name);
    if (!name[0]) {
        strcpy(outbuf, "");
        return outbuf;
    } else {
        int count = 1;

        for (rch = vch->in_room->people; rch && (rch != vch); rch = rch->next_in_room)
            if (is_name(name, rch->name))
                count++;

        sprintf(outbuf, "%d.%s", count, name);
    }

    return outbuf;
}
