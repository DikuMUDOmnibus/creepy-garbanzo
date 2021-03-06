#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "interp.h"
#include "olc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "help.h"



/***************************************************************************
 *	local functions
 ***************************************************************************/
static bool set_integer_arg(int *value, const char *argument);
static bool set_uint_arg(unsigned int *value, const char *argument);
static bool set_long_arg(long *value, const char *argument);
static bool set_obj_value_idx(struct gameobject * obj, int idx, const char *argument);
static void item_type_help(struct char_data * ch, int item_type);

/***************************************************************************
 *	set functions
 ***************************************************************************/
typedef void SET_FN(struct char_data * ch, const char *argument);
typedef bool SET_ROOM_FN(struct char_data * ch, struct roomtemplate * room, const char *argument);
typedef bool SET_CHAR_FN(struct char_data * ch, struct char_data * vch, const char *argument);
typedef bool SET_OBJ_FN(struct char_data * ch, struct gameobject * obj, const char *argument);

static SET_FN set_character;
static SET_FN set_object;
static SET_FN set_room;
static SET_FN set_skill;
static SET_FN set_reboot;
static SET_FN set_copyover;

/***************************************************************************
 *	set_cmd_table
 ***************************************************************************/
static const struct set_cmd_type {
    char *	keyword;
    SET_FN *fn;
}
set_cmd_table[] =
{
    { "mobile",    set_character },
    { "character", set_character },
    { "object",    set_object    },
    { "room",      set_room	     },
    { "skill",     set_skill     },
    { "spell",     set_skill     },
    { "reboot",    set_reboot    },
    { "copyover",  set_copyover  },
    { "",	       NULL	     }
};



/***************************************************************************
 *	do_set
 *
 *	entry level function for set commands
 ***************************************************************************/
void do_set(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    int idx;

    DENY_NPC(ch);

    argument = one_argument(argument, arg);

    if (arg[0] == '\0') {
        show_help(ch->desc, "SET_CMD_SYNTAX", NULL);
        return;
    }

    for (idx = 0; set_cmd_table[idx].keyword[0] != '\0'; idx++) {
        if (!str_prefix(arg, set_cmd_table[idx].keyword)) {
            (*set_cmd_table[idx].fn)(ch, argument);
            return;
        }
    }

    do_set(ch, "");
}



/***************************************************************************
 *	characters/mobs
 ***************************************************************************/
static SET_CHAR_FN set_char_str;
static SET_CHAR_FN set_char_int;
static SET_CHAR_FN set_char_wis;
static SET_CHAR_FN set_char_dex;
static SET_CHAR_FN set_char_con;
static SET_CHAR_FN set_char_luck;
static SET_CHAR_FN set_char_sex;
static SET_CHAR_FN set_char_class;
static SET_CHAR_FN set_char_race;
static SET_CHAR_FN set_char_hp;
static SET_CHAR_FN set_char_mana;
static SET_CHAR_FN set_char_move;
static SET_CHAR_FN set_char_train;
static SET_CHAR_FN set_char_practice;
/* money */
static SET_CHAR_FN set_char_gold;
static SET_CHAR_FN set_char_silver;
static SET_CHAR_FN set_char_bank_gold;
static SET_CHAR_FN set_char_bank_silver;
/* kills/deaths */
static SET_CHAR_FN set_char_pkills;
static SET_CHAR_FN set_char_pdeaths;
static SET_CHAR_FN set_char_mobkills;
static SET_CHAR_FN set_char_mobdeaths;
/* conditions */
static SET_CHAR_FN set_char_full;
SET_CHAR_FN set_char_hunger;
SET_CHAR_FN set_char_thirst;
SET_CHAR_FN set_char_feed;
/* misc properties */
static SET_CHAR_FN set_char_deathroom;
static SET_CHAR_FN set_char_security;
static SET_CHAR_FN set_char_reply;
static SET_CHAR_FN set_char_level;
static SET_CHAR_FN set_char_memory;
static SET_CHAR_FN set_char_extendedexp;

static const struct set_char_cmd_type {
    char *		keyword;
    SET_CHAR_FN *	fn;
}
set_char_cmd_table[] =
{
    /* general stats */
    { "str",	  set_char_str		},
    { "int",	  set_char_int		},
    { "wis",	  set_char_wis		},
    { "dex",	  set_char_dex		},
    { "con",	  set_char_con		},
    { "luck",	  set_char_luck		},
    { "sex",	  set_char_sex		},
    { "class",	  set_char_class	},
    { "race",	  set_char_race		},
    { "hp",		  set_char_hp		},
    { "mana",	  set_char_mana		},
    { "move",	  set_char_move		},
    { "train",	  set_char_train	},
    { "practice",	  set_char_practice	},
    /* money */
    { "gold",	  set_char_gold		},
    { "silver",	  set_char_silver	},
    { "bank_gold",	  set_char_bank_gold	},
    { "bank_silver",  set_char_bank_silver	},
    /* kills/deaths */
    { "pkills",	  set_char_pkills	},
    { "pdeaths",	  set_char_pdeaths	},
    { "mobkills",	  set_char_mobkills	},
    { "mobdeaths",	  set_char_mobdeaths	},
    /* conditions */
    { "full",	  set_char_full		},
    { "hunger",	  set_char_hunger	},
    { "thirst",	  set_char_thirst	},
    { "feed",	  set_char_feed		},
    /* misc properties */
    { "deathroom",	  set_char_deathroom	},
    { "security",	  set_char_security	},
    { "reply",	  set_char_reply	},
    { "level",	  set_char_level	},
    { "memory",	  set_char_memory	},
    { "extendedexp",  set_char_extendedexp	},
    { "",		  NULL			}
};


/***************************************************************************
 *	set_character
 *
 *	set properties on a character data structure
 ***************************************************************************/
static void set_character(struct char_data *ch, const char *argument)
{
    struct char_data *vch;
    char arg[MAX_INPUT_LENGTH];
    char cmd[MAX_INPUT_LENGTH];
    int idx;

    DENY_NPC(ch);

    if (is_help(argument)) {
        int col;

        send_to_char("`#SYNTAX``: set char <name> <field> <value>\n\r\n\r", ch);
        send_to_char("Available fields:\n\r   ", ch);
        col = 0;
        for (idx = 0; set_char_cmd_table[idx].keyword[0] != '\0'; idx++) {
            printf_to_char(ch, "%-15.15s", set_char_cmd_table[idx].keyword);
            if (++col % 3 == 0)
                send_to_char("\n\r   ", ch);
        }

        if (col % 4 != 0)
            send_to_char("\n\r", ch);

        return;
    }


    argument = one_argument(argument, arg);
    if ((vch = get_char_world(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    {
        static char buf[MAX_STRING_LENGTH];
        argument = one_argument(argument, cmd);
        (void)snprintf(buf, UMIN(strlen(argument), MAX_STRING_LENGTH), "%s", argument);
        smash_tilde(buf);

        if (cmd[0] != '\0') {
            for (idx = 0; set_char_cmd_table[idx].keyword[0] != '\0'; idx++) {
                if (!str_prefix(cmd, set_char_cmd_table[idx].keyword)) {
                    if ((*set_char_cmd_table[idx].fn)(ch, vch, argument))
                        send_to_char("`1Ok`!.``\n\r", ch);
                    return;
                }
            }
        }
    }

    /* generate help message */
    set_character(ch, "");
    return;
}



/***************************************************************************
 *	general character sets
 ***************************************************************************/
/***************************************************************************
 *	set_char_str
 *
 *	set the strength of a character
 ***************************************************************************/
static bool set_char_str(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: str [+/-]<number>\n\r", ch);
        return false;
    }

    set_integer_arg(&vch->perm_stat[STAT_STR], argument);
    vch->perm_stat[STAT_STR] = URANGE(3, vch->perm_stat[STAT_STR], get_max_train(vch, STAT_STR));
    return true;
}

/***************************************************************************
 *	set_char_int
 *
 *	set the dexterity of a character
 ***************************************************************************/
static bool set_char_int(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: int [+/-]<number>\n\r", ch);
        return false;
    }

    set_integer_arg(&vch->perm_stat[STAT_INT], argument);
    vch->perm_stat[STAT_INT] = URANGE(3, vch->perm_stat[STAT_INT], get_max_train(vch, STAT_INT));
    return true;
}

/***************************************************************************
 *	set_char_wis
 *
 *	set the wisdom of a character
 ***************************************************************************/
static bool set_char_wis(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: wis [+/-]<number>\n\r", ch);
        return false;
    }

    set_integer_arg(&vch->perm_stat[STAT_WIS], argument);
    vch->perm_stat[STAT_WIS] = URANGE(3, vch->perm_stat[STAT_WIS], get_max_train(vch, STAT_WIS));
    return true;
}


/***************************************************************************
 *	set_char_dex
 *
 *	set the dexterity of a character
 ***************************************************************************/
static bool set_char_dex(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: dex [+/-]<number>\n\r", ch);
        return false;
    }

    set_integer_arg(&vch->perm_stat[STAT_DEX], argument);
    vch->perm_stat[STAT_DEX] = URANGE(3, vch->perm_stat[STAT_DEX], get_max_train(vch, STAT_DEX));
    return true;
}

/***************************************************************************
 *	set_char_con
 *
 *	set the constitution of a character
 ***************************************************************************/
static bool set_char_con(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: con [+/-]<number>\n\r", ch);
        return false;
    }

    set_integer_arg(&vch->perm_stat[STAT_CON], argument);
    vch->perm_stat[STAT_CON] = URANGE(3, vch->perm_stat[STAT_CON], get_max_train(vch, STAT_CON));
    return true;
}

/***************************************************************************
 *	set_char_luck
 *
 *	set the luck of a character
 ***************************************************************************/
static bool set_char_luck(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: luck [+/-]<number>\n\r", ch);
        return false;
    }

    set_integer_arg(&vch->perm_stat[STAT_LUCK], argument);
    vch->perm_stat[STAT_LUCK] = URANGE(3, vch->perm_stat[STAT_LUCK], get_max_train(vch, STAT_LUCK));
    return true;
}


/***************************************************************************
 *	set_char_sex
 *
 *	set the sex of a character
 ***************************************************************************/
static bool set_char_sex(struct char_data *ch, struct char_data *vch, const char *argument)
{
    int value;

    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: sex <sex name>\n\r", ch);
        return false;
    }

    value = sex_lookup(argument);
    if (value < 0 || value > 2) {
        send_to_char("That is not a valid sex.\n\r", ch);
        return false;
    }

    vch->sex = value;
    if (!IS_NPC(vch))
        vch->pcdata->true_sex = value;
    return true;
}


/***************************************************************************
 *	set_char_race
 *
 *	set the race of a character
 ***************************************************************************/
static bool set_char_race(struct char_data *ch, struct char_data *vch, const char *argument)
{
    int value;
    char buf[MAX_STRING_LENGTH];
    bool gobbed = false;

    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: race <race name>\n\r", ch);
        return false;
    }

    value = race_lookup(argument);
    if (value == -1) {
        send_to_char("That is not a valid race.\n\r", ch);
        return false;
    }

    if (!IS_NPC(vch) && !race_table[value].pc_race) {
        send_to_char("That is not a valid player race.\n\r", ch);
        return false;
    }

    /* no sense in beating a dead horse...
     *  added by Monrick, 1/2008            */
    if (vch->race == value) {
        printf_to_char(ch, "%s is already a %s!\n\r", capitalize(IS_NPC(vch) ? vch->short_descr : vch->name), race_table[value].name);
        return false;
    }

    /* check for permanent racial specials to remove
     * added by Monrick, 1/2008            */
    if (!IS_NPC(vch)) {
        sprintf(buf, "Changing %s from ", capitalize(vch->name));
        strcat(buf, capitalize(race_table[vch->race].name));
        strcat(buf, " to ");
        strcat(buf, capitalize(race_table[value].name));
        strcat(buf, ".\n\r");
        send_to_char(buf, ch);

        printf_to_char(ch, "  Removing %s special effects...\n\r", race_table[vch->race].name);

        /* when you're not a vampire anymore, there's no need
         *         to feed... auto-checks for gobstopper */

        printf_to_char(ch, "  All %s special effects removed.\n\r", race_table[vch->race].name);
        printf_to_char(vch, "Your body morphs from a %s into a ", capitalize(race_table[vch->race].name));
        printf_to_char(vch, "%s.\n\r", capitalize(race_table[value].name));
    }

    vch->race = value;

    if (!IS_NPC(vch)) {
        vch->exp = (exp_per_level(vch, vch->pcdata->points) * vch->level);

        sprintf(buf, "$N is now a %s.",
                capitalize(race_table[vch->race].name));
        act(buf, ch, NULL, vch, TO_CHAR);

        printf_to_char(ch, "  Adding %s special effects...\n\r", race_table[vch->race].name);

        /* now that you're a vampire, you'll need to feed instead of
         * ... auto-checks for gobstopper */

        if (gobbed)
            send_to_char("Your `OG`@o`1b`#s`Pt`@o`1p`#p`Pe`Or`` effect is still in place...\n\r", vch);

        printf_to_char(ch, "  All %s special effects added.\n\r", race_table[vch->race].name);
        do_save(vch, "");
    }

    return true;
}

/***************************************************************************
 *	set_char_hp
 *
 *	set the hp of a character
 ***************************************************************************/
static bool set_char_hp(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: hp [+/-]<number>\n\r", ch);
        return false;
    }

    set_integer_arg(&vch->max_hit, argument);
    vch->max_hit = UMAX(1, vch->max_hit);

    if (!IS_NPC(vch)) {
        set_integer_arg(&vch->pcdata->perm_hit, argument);
        vch->pcdata->perm_hit = UMAX(1, vch->pcdata->perm_hit);
    }
    return true;
}

/***************************************************************************
 *	set the mana of a character
 ***************************************************************************/
static bool set_char_mana(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: mana [+/-]<number>\n\r", ch);
        return false;
    }

    set_integer_arg(&vch->max_mana, argument);
    vch->max_mana = UMAX(1, vch->max_mana);

    if (!IS_NPC(vch)) {
        set_integer_arg(&vch->pcdata->perm_mana, argument);
        vch->pcdata->perm_mana = UMAX(1, vch->pcdata->perm_mana);
    }
    return true;
}

/***************************************************************************
 *	set the move of a character
 ***************************************************************************/
static bool set_char_move(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: move [+/-]<number>\n\r", ch);
        return false;
    }

    set_integer_arg(&vch->max_move, argument);
    vch->max_move = UMAX(1, vch->max_move);

    if (!IS_NPC(vch)) {
        set_integer_arg(&vch->pcdata->perm_move, argument);
        vch->pcdata->perm_move = UMAX(1, vch->pcdata->perm_move);
    }
    return true;
}

/***************************************************************************
 *	set the gold property of a character
 ***************************************************************************/
static bool set_char_gold(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: gold [+/-]<number>\n\r", ch);
        return false;
    }

    set_uint_arg(&vch->gold, argument);
    vch->gold = UMAX(vch->gold, 0);
    return true;
}


/***************************************************************************
 *	set the silver in bank property of a character
 ***************************************************************************/
static bool set_char_silver(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: silver [+/-]<number>\n\r", ch);
        return false;
    }

    set_uint_arg(&vch->silver, argument);
    vch->silver = UMAX(vch->silver, 0);
    return true;
}


/***************************************************************************
 *	set_char_reply
 *
 *	set the reply property for a character
 ***************************************************************************/
static bool set_char_reply(struct char_data *ch, struct char_data *vch, const char *argument)
{
    struct char_data *reply;

    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: reply <character to reply to>\n\r", ch);
        return false;
    }

    if ((reply = get_char_world(ch, argument)) == NULL) {
        send_to_char("That character does not exist.\n\r", ch);
        return false;
    }

    vch->reply = reply;
    return true;
}


/***************************************************************************
 *	player-specific sets
 ***************************************************************************/
/***************************************************************************
 *	set_char_class
 *
 *	set the class of a character
 ***************************************************************************/
static bool set_char_class(struct char_data *ch, struct char_data *vch, const char *argument)
{
    int value;

    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: class <class name>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    value = class_lookup(argument);
    if (value == -1) {
        send_to_char("That is not a valid class. Valid classes are:\n\r", ch);
        for (value = 0; value < MAX_CLASS; value++)

            printf_to_char(ch, "  %s", class_table[value].name);
        send_to_char("\n\r", ch);
        return false;
    }

    vch->class = value;
    vch->exp = (exp_per_level(vch, vch->pcdata->points) * vch->level);
    return true;
}

/***************************************************************************
 *	set_char_train
 *
 *	set the number of trains for a player
 ***************************************************************************/
static bool set_char_train(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: train [+/-]<number>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    set_integer_arg(&vch->pcdata->train, argument);
    vch->pcdata->train = UMAX(0, vch->pcdata->train);
    return true;
}

/***************************************************************************
 *	set_char_practice
 *
 *	set the number of practices for a player
 ***************************************************************************/
static bool set_char_practice(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: practice [+/-]<number>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    set_integer_arg(&vch->pcdata->practice, argument);
    vch->pcdata->practice = UMAX(0, vch->pcdata->practice);
    return true;
}

/***************************************************************************
 *	set_char_bank_gold
 *
 *	set the gold in bank property of a player
 ***************************************************************************/
static bool set_char_bank_gold(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: bank_gold [+/-]<number>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    set_uint_arg(&vch->pcdata->gold_in_bank, argument);
    vch->pcdata->gold_in_bank = UMAX(vch->pcdata->gold_in_bank, 0);
    return true;
}


/***************************************************************************
 *	set_char_bank_silver
 *
 *	set the silver in bank property of a player
 ***************************************************************************/
static bool set_char_bank_silver(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: bank_silver [+/-]<number>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    set_uint_arg(&vch->pcdata->silver_in_bank, argument);
    vch->pcdata->silver_in_bank = UMAX(vch->pcdata->silver_in_bank, 0);
    return true;
}

/***************************************************************************
 *	set_char_pkills
 *
 *	set the player kills property of a player
 ***************************************************************************/
static bool set_char_pkills(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: pkills [+/-]<number>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    set_long_arg(&vch->pcdata->pkills, argument);
    vch->pcdata->pkills = UMAX(vch->pcdata->pkills, 0);
    return true;
}


/***************************************************************************
 *	set_char_pdeaths
 *
 *	set the player deaths property of a player
 ***************************************************************************/
static bool set_char_pdeaths(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: pdeaths [+/-]<number>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    set_long_arg(&vch->pcdata->pdeaths, argument);
    vch->pcdata->pdeaths = UMAX(vch->pcdata->pdeaths, 0);
    return true;
}


/***************************************************************************
 *	set_char_mobkills
 *
 *	set the mob kills property of a player
 ***************************************************************************/
static bool set_char_mobkills(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: mobkills [+/-]<number>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    set_long_arg(&vch->pcdata->mobkills, argument);
    vch->pcdata->mobkills = UMAX(vch->pcdata->mobkills, 0);
    return true;
}


/***************************************************************************
 *	set_char_mobdeaths
 *
 *	set the mob deaths property of a player
 ***************************************************************************/
static bool set_char_mobdeaths(struct char_data *ch, struct char_data *vch, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: mobdeaths [+/-]<number>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    set_long_arg(&vch->pcdata->mobdeaths, argument);
    vch->pcdata->mobdeaths = UMAX(vch->pcdata->mobdeaths, 0);
    return true;
}


/***************************************************************************
 *	set_char_full
 *
 *	set the full condition of a player
 ***************************************************************************/
static bool set_char_full(struct char_data *ch, struct char_data *vch, const char *argument)
{
    int value;

    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: full [+/-]<number of ticks>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    if (!is_number(argument)) {
        send_to_char("The supplied value must be numeric.\n\r", ch);
        return false;
    }

    value = parse_int(argument);

    if (value < -151 || value > 100) {
        send_to_char("The value must be between -151 and 100.\n\r", ch);
        return false;
    }

    if (value == -151 && get_trust(ch) < 609) {
        send_to_char("Only Implementors may set permanent values.\n\r", ch);
        return false;
    }
    vch->pcdata->condition[COND_FULL] = value;
    return true;
}


/***************************************************************************
 *	set_char_hunger
 *
 *	set the hunger condition of a player
 ***************************************************************************/
bool set_char_hunger(struct char_data *ch, struct char_data *vch, const char *argument)
{
    int value;

    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: hunger [+/-]<number of ticks>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    if (!is_number(argument)) {
        send_to_char("The supplied value must be numeric.\n\r", ch);
        return false;
    }

    value = parse_int(argument);
    if (value < -151 || value > 100) {
        send_to_char("The value must be between -151 and 100.\n\r", ch);
        return false;
    }

    if (value == -151 && get_trust(ch) < 609) {
        send_to_char("Only Implementors may set permanent values.\n\r", ch);
        return false;
    }
    vch->pcdata->condition[COND_HUNGER] = value;
    return true;
}


/***************************************************************************
 *	set_char_thirst
 *
 *	set the thirst condition of a player
 ***************************************************************************/
bool set_char_thirst(struct char_data *ch, struct char_data *vch, const char *argument)
{
    int value;

    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: thirst [+/-]<number of ticks>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    if (!is_number(argument)) {
        send_to_char("The supplied value must be numeric.\n\r", ch);
        return false;
    }

    value = parse_int(argument);
    if (value < -151 || value > 100) {
        send_to_char("The value must be between -151 and 100.\n\r", ch);
        return false;
    }

    if (value == -151 && get_trust(ch) < 609) {
        send_to_char("Only Implementors may set permanent values.\n\r", ch);
        return false;
    }
    vch->pcdata->condition[COND_THIRST] = value;
    return true;
}

/***************************************************************************
 *	set_char_feed
 *
 *	set the feed condition of a player
 ***************************************************************************/
bool set_char_feed(struct char_data *ch, struct char_data *vch, const char *argument)
{
    int value;

    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: feed [+/-]<number of ticks>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    if (!is_number(argument)) {
        send_to_char("The supplied value must be numeric.\n\r", ch);
        return false;
    }

    value = parse_int(argument);
    if (value < -151 || value > 100) {
        send_to_char("The value must be between -151 and 100.\n\r", ch);
        return false;
    }

    if (value == -151 && get_trust(ch) < 609) {
        send_to_char("Only Implementors may set permanent values.\n\r", ch);
        return false;
    }
    vch->pcdata->condition[COND_FEED] = value;
    return true;
}


/***************************************************************************
 *	set_char_deathroom
 *
 *	set the deathroom room for a player
 ***************************************************************************/
static bool set_char_deathroom(struct char_data *ch, struct char_data *vch, const char *argument)
{
    struct roomtemplate *room;
    int value;

    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: deathroom <room vnum>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    value = parse_int(argument);
    if (value != 0
        && (room = get_room_index(value)) == NULL) {
        send_to_char("That room does not exist.\n\r", ch);
        return false;
    }

    vch->deathroom = value;
    return true;
}

/***************************************************************************
 *	set_char_security
 *
 *	set the security property of a player
 ***************************************************************************/
static bool set_char_security(struct char_data *ch, struct char_data *vch, const char *argument)
{
    int value;

    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: security <0-9>\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on mobs.\n\r", ch);
        return false;
    }

    value = parse_int(argument);
    if (value < 0 || value > 9) {
        send_to_char("Security must be between 0 and 9.\n\r", ch);
        return false;
    }

    vch->pcdata->security = value;
    return true;
}






/***************************************************************************
 *	set_char_extendedexp
 *
 *	set the extended exp for a pc (added by Monrick, May 2008)
 ***************************************************************************/
static bool set_char_extendedexp(struct char_data *ch, struct char_data *vch, const char *argument)
{
    int plusminus = 0;
    int value;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: extendedexp <value> [+, - or blank to set a #]\n\r", ch);
        return false;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on NPCs.\n\r", ch);
        return false;
    }

    if (get_trust(ch) < MAX_LEVEL) {
        send_to_char("I don't care if you have \"set\" or not, only `2I`3M`2P``s may set ExtendedExp.\n\r", ch);
        return false;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0') {
        send_to_char("`#SYNTAX``: extendedexp <value> [+, - or blank to set a #]\n\r", ch);
        return false;
    }

    if (arg2[0] == '+')
        plusminus = 1;
    else if (arg2[0] == '-')
        plusminus = -1;

    value = parse_int(arg1);

    if (plusminus != 0)
        vch->pcdata->extendedexp += (plusminus * value);
    else
        vch->pcdata->extendedexp = abs(value);

    return true;
}


/***************************************************************************
 *	mob-specific sets
 ***************************************************************************/
/***************************************************************************
 *	set_char_level
 *
 *	set the level property for a mob
 ***************************************************************************/
static bool set_char_level(struct char_data *ch, struct char_data *vch, const char *argument)
{
    int value;

    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: level <level number>\n\r", ch);
        return false;
    }

    if (!IS_NPC(vch)) {
        send_to_char("Not on players.\n\r", ch);
        return false;
    }

    value = parse_int(argument);
    if (value <= 0) {
        send_to_char("The level must be a positive number.\n\r", ch);
        return false;
    }

    vch->level = value;
    return true;
}


/***************************************************************************
 *	set_char_memory
 *
 *	set the memory property for a mob
 ***************************************************************************/
static bool set_char_memory(struct char_data *ch, struct char_data *vch, const char *argument)
{
    struct char_data *mem;

    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: memory <character to remember>\n\r", ch);
        return false;
    }

    if (!IS_NPC(vch)) {
        send_to_char("Not on players.\n\r", ch);
        return false;
    }

    if (!str_cmp(argument, "none")) {
        vch->mobmem = NULL;

        send_to_char("Your target is now happy-happy-joy-joy.\n\r", ch);
    } else {
        if ((mem = get_char_world(vch, argument)) == NULL) {
            send_to_char("Mob couldn't locate the victim to remember.\n\r", ch);
            return false;
        }

        vch->mobmem = mem;
    }

    return true;
}


/***************************************************************************
 *	objects
 ***************************************************************************/
static SET_OBJ_FN set_obj_v0;
static SET_OBJ_FN set_obj_v1;
static SET_OBJ_FN set_obj_v2;
static SET_OBJ_FN set_obj_v3;
static SET_OBJ_FN set_obj_v4;
static SET_OBJ_FN set_obj_extra;
static SET_OBJ_FN set_obj_extra2;
static SET_OBJ_FN set_obj_weight;
static SET_OBJ_FN set_obj_cost;
static SET_OBJ_FN set_obj_timer;


static const struct set_obj_cmd_type {
    char *		keyword;
    SET_OBJ_FN *	fn;
}
set_obj_cmd_table[] =
{
    /* general stats */
    { "v0",	    set_obj_v0	   },
    { "v1",	    set_obj_v1	   },
    { "v2",	    set_obj_v2	   },
    { "v3",	    set_obj_v3	   },
    { "v4",	    set_obj_v4	   },
    { "extra",  set_obj_extra  },
    { "extra2", set_obj_extra2 },
    { "weight", set_obj_weight },
    { "cost",   set_obj_cost   },
    { "timer",  set_obj_timer  },
    { "",	    NULL	   }
};


/***************************************************************************
 *	set_object
 *
 *	set a property on an object
 ***************************************************************************/
static void set_object(struct char_data *ch, const char *argument)
{
    struct gameobject *obj;
    char arg[MAX_INPUT_LENGTH];
    char cmd[MAX_INPUT_LENGTH];
    int idx;

    DENY_NPC(ch);

    if (is_help(argument)) {
        int col;

        send_to_char("`#SYNTAX``: set object <object name> <field> <value>\n\r\n\r", ch);
        send_to_char("Available fields:\n\r   ", ch);
        col = 0;
        for (idx = 0; set_obj_cmd_table[idx].keyword[0] != '\0'; idx++) {
            printf_to_char(ch, "%-15.15s", set_obj_cmd_table[idx].keyword);
            if (++col % 3 == 0)
                send_to_char("\n\r   ", ch);
        }

        if (col % 3 != 0)
            send_to_char("\n\r", ch);

        return;
    }

    argument = one_argument(argument, arg);
    if ((obj = get_obj_here(ch, arg)) == NULL) {
        send_to_char("That object is not in this room or in your inventory.\n\r", ch);
        return;
    }

    {
        static char buf[MAX_STRING_LENGTH];
        argument = one_argument(argument, cmd);
        (void)snprintf(buf, UMIN(strlen(argument), MAX_STRING_LENGTH), "%s", argument);
        smash_tilde(buf);

        if (cmd[0] != '\0') {
            for (idx = 0; set_obj_cmd_table[idx].keyword[0] != '\0'; idx++) {
                if (!str_prefix(cmd, set_obj_cmd_table[idx].keyword)) {
                    if ((*set_obj_cmd_table[idx].fn)(ch, obj, buf))
                        send_to_char("`1Ok`!.``\n\r", ch);
                    return;
                }
            }
        }
    }

    /* generate help message */
    set_object(ch, "");
    return;
}



/***************************************************************************
 *	set_obj_v0
 *
 *	set the value 0 property of an object
 ***************************************************************************/
static bool set_obj_v0(struct char_data *ch, struct gameobject *obj, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: v0 <value>\n\r\n\r", ch);
        item_type_help(ch, OBJECT_TYPE(obj));
        return false;
    }

    if (!set_obj_value_idx(obj, 0, argument)) {
        item_type_help(ch, OBJECT_TYPE(obj));
        return false;
    }

    return true;
}


/***************************************************************************
 *	set_obj_v1
 *
 *	set the value 1 property of an object
 ***************************************************************************/
static bool set_obj_v1(struct char_data *ch, struct gameobject *obj, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: v1 <value>\n\r\n\r", ch);
        item_type_help(ch, OBJECT_TYPE(obj));
        return false;
    }

    if (!set_obj_value_idx(obj, 1, argument)) {
        item_type_help(ch, OBJECT_TYPE(obj));
        return false;
    }
    return true;
}


/***************************************************************************
 *	set_obj_v2
 *
 *	set the value 2 property of an object
 ***************************************************************************/
static bool set_obj_v2(struct char_data *ch, struct gameobject *obj, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: v2 <value>\n\r\n\r", ch);
        item_type_help(ch, OBJECT_TYPE(obj));
        return false;
    }

    if (!set_obj_value_idx(obj, 2, argument)) {
        item_type_help(ch, OBJECT_TYPE(obj));
        return false;
    }

    return true;
}

/***************************************************************************
 *	set_obj_v3
 *
 *	set the value 3 property of an object
 ***************************************************************************/
static bool set_obj_v3(struct char_data *ch, struct gameobject *obj, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: v3 <value>\n\r\n\r", ch);
        item_type_help(ch, OBJECT_TYPE(obj));
        return false;
    }

    if (!set_obj_value_idx(obj, 3, argument)) {
        item_type_help(ch, OBJECT_TYPE(obj));
        return false;
    }

    return true;
}


/***************************************************************************
 *	set_obj_v4
 *
 *	set the value 0 property of an object
 ***************************************************************************/
static bool set_obj_v4(struct char_data *ch, struct gameobject *obj, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: v4 <value>\n\r\n\r", ch);
        item_type_help(ch, OBJECT_TYPE(obj));
        return false;
    }

    if (!set_obj_value_idx(obj, 4, argument)) {
        item_type_help(ch, OBJECT_TYPE(obj));
        return false;
    }

    return true;
}

/***************************************************************************
 *	set_obj_extra
 *
 *	set the extra property of an object
 ***************************************************************************/
static bool set_obj_extra(struct char_data *ch, struct gameobject *obj, const char *argument)
{
    if (is_help(argument)) {
        int idx;
        int col;

        send_to_char("`#SYNTAX``: extra <numeric value|flag value>\n\r", ch);
        send_to_char("Available flag values:\n\r   ", ch);
        col = 0;
        for (idx = 0; extra_flags[idx].name != NULL; idx++) {
            printf_to_char(ch, "%-15.15s", extra_flags[idx].name);
            if (++col % 3 == 0)
                send_to_char("\n\r   ", ch);
        }

        if (col % 3 != 0)
            send_to_char("\n\r", ch);
    }

    if (is_number(argument)) {
        obj->extra_flags = parse_long(argument);
    } else {
        long value;

        if ((value = flag_value(extra_flags, argument)) == NO_FLAG) {
            send_to_char("Those flags are not settable.\n\r", ch);
            return false;
        }

        if (*argument == '+')
            SET_BIT(obj->extra_flags, value);
        else if (*argument == '-')
            REMOVE_BIT(obj->extra_flags, value);
        else
            obj->extra_flags = value;
    }
    return true;
}

/***************************************************************************
 *       set_obj_extra2
 *
 *       set the extra2 property of an object
 ***************************************************************************/
static bool set_obj_extra2(struct char_data *ch, struct gameobject *obj, const char *argument)
{
    if (is_help(argument)) {
        int idx;
        int col;

        send_to_char("`#SYNTAX``: extra2 <numeric value|flag value>\n\r", ch);
        send_to_char("Available flag values:\n\r   ", ch);
        col = 0;
        for (idx = 0; extra2_flags[idx].name != NULL; idx++) {
            printf_to_char(ch, "%-15.15s", extra2_flags[idx].name);
            if (++col % 3 == 0)
                send_to_char("\n\r   ", ch);
        }

        if (col % 3 != 0)
            send_to_char("\n\r", ch);
    }

    if (is_number(argument)) {
        obj->extra2_flags = parse_long(argument);
    } else {
        long value;

        if ((value = flag_value(extra2_flags, argument)) == NO_FLAG) {
            send_to_char("Those flags are not settable.\n\r", ch);
            return false;
        }

        if (*argument == '+')
            SET_BIT(obj->extra2_flags, value);
        else if (*argument == '-')
            REMOVE_BIT(obj->extra2_flags, value);
        else
            obj->extra2_flags = value;
    }
    return true;
}


/***************************************************************************
 *	set_obj_cost
 *
 *	set the cost of an object
 ***************************************************************************/
static bool set_obj_cost(struct char_data *ch, struct gameobject *obj, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: cost <amount>\n\r", ch);
        return false;
    }

    set_uint_arg(&obj->cost, argument);
    obj->cost = UMAX(obj->cost, 0);
    return true;
}


/***************************************************************************
 *	set_obj_weight
 *
 *	set the weight of an object
 ***************************************************************************/
static bool set_obj_weight(struct char_data *ch, struct gameobject *obj, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: weight <amount>\n\r", ch);
        return false;
    }

    set_integer_arg(&obj->weight, argument);
    obj->weight = UMAX(obj->weight * 10, 0);
    return true;
}


/***************************************************************************
 *	set_obj_timer
 *
 *	set the light value of a room
 ***************************************************************************/
static bool set_obj_timer(struct char_data *ch, struct gameobject *obj, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: timer <number of ticks>\n\r", ch);
        return false;
    }

    set_integer_arg(&obj->timer, argument);
    obj->timer = UMAX(obj->timer, -1);
    return true;
}





/***************************************************************************
 *	rooms
 ***************************************************************************/
static SET_ROOM_FN set_room_flags;
static SET_ROOM_FN set_room_sector;
static SET_ROOM_FN set_room_mana_rate;
static SET_ROOM_FN set_room_heal_rate;
static SET_ROOM_FN set_room_light;


static const struct set_room_cmd_type {
    char *		keyword;
    SET_ROOM_FN *	fn;
}
set_room_cmd_table[] =
{
    /* general stats */
    { "flags",     set_room_flags	  },
    { "sector",    set_room_sector	  },
    { "mana_rate", set_room_mana_rate },
    { "heal_rate", set_room_heal_rate },
    { "light",     set_room_light	  },
    { "",	       NULL		  }
};

/***************************************************************************
 *	set_room
 *
 *	set a property on a room structure
 ***************************************************************************/
static void set_room(struct char_data *ch, const char *argument)
{
    struct roomtemplate *room;
    char arg[MAX_INPUT_LENGTH];
    char cmd[MAX_INPUT_LENGTH];
    int idx;

    DENY_NPC(ch);

    if (is_help(argument)) {
        int col;

        send_to_char("`#SYNTAX``: set room [vnum] <field> <value>\n\r\n\r", ch);
        send_to_char("Available fields:\n\r   ", ch);
        col = 0;
        for (idx = 0; set_room_cmd_table[idx].keyword[0] != '\0'; idx++) {
            printf_to_char(ch, "%-15.15s", set_room_cmd_table[idx].keyword);
            if (++col % 3 == 0)
                send_to_char("\n\r   ", ch);
        }

        if (col % 4 != 0)
            send_to_char("\n\r", ch);

        return;
    }

    one_argument(argument, arg);
    if (is_number(arg) && (room = get_room_index(parse_int(arg))) != NULL)
        argument = one_argument(argument, arg);
    else
        room = ch->in_room;

    {
        static char buf[MAX_STRING_LENGTH];
        argument = one_argument(argument, cmd);
        (void)snprintf(buf, UMIN(strlen(argument), MAX_STRING_LENGTH), "%s", argument);
        smash_tilde(buf);
        if (cmd[0] != '\0') {
            for (idx = 0; set_room_cmd_table[idx].keyword[0] != '\0'; idx++) {
                if (!str_prefix(cmd, set_room_cmd_table[idx].keyword)) {
                    if ((*set_room_cmd_table[idx].fn)(ch, room, buf))
                        send_to_char("`1Ok`!.``\n\r", ch);
                    return;
                }
            }
        }
    }

    /* generate help message */
    set_room(ch, "");
}



/***************************************************************************
 *	general room set functions
 ***************************************************************************/
/***************************************************************************
 *	set_room_flags
 *
 *	set the flags on a room
 ***************************************************************************/
static bool set_room_flags(struct char_data *ch, struct roomtemplate *room, const char *argument)
{
    long value;

    if (is_help(argument)) {
        int idx;
        int col;

        send_to_char("`#SYNTAX``: flags [+|-]<flag name list>\n\r\n\r", ch);
        send_to_char("Available values:\n\r   ", ch);
        col = 0;
        for (idx = 0; room_flags[idx].name != NULL; idx++) {
            printf_to_char(ch, "%-15.15s", room_flags[idx].name);
            if (++col % 3 == 0)
                send_to_char("\n\r   ", ch);
        }

        if (col % 3 != 0)
            send_to_char("\n\r", ch);

        return false;
    }

    if ((value = flag_value(room_flags, argument)) == NO_FLAG) {
        send_to_char("Those flags do not exist.\n\r", ch);
        return false;
    } else {
        if (*argument == '+') {
            SET_BIT(room->room_flags, value);
        } else if (*argument == '-') {
            REMOVE_BIT(room->room_flags, value);
        } else {
            room->room_flags = value;;
        }
    }

    return true;
}



/***************************************************************************
 *	set_room_sector
 *
 *	set the sector of a room
 ***************************************************************************/
static bool set_room_sector(struct char_data *ch, struct roomtemplate *room, const char *argument)
{
    int value;

    if (is_help(argument)) {
        int idx;
        int col;

        send_to_char("`#SYNTAX``: sector <sector name >\n\r\n\r", ch);
        send_to_char("Available values:\n\r   ", ch);
        col = 0;
        for (idx = 0; sector_flags[idx].name != NULL; idx++) {
            printf_to_char(ch, "%-11.11s", sector_flags[idx].name);
            if (++col % 4 == 0)
                send_to_char("\n\r   ", ch);
        }

        if (col % 4 != 0)
            send_to_char("\n\r", ch);

        return false;
    }

    if ((value = flag_value(sector_flags, argument)) == NO_FLAG) {
        send_to_char("That sector type does not exist.\n\r", ch);
        return false;
    }

    room->sector_type = value;
    return true;
}

/***************************************************************************
 *	set_room_mana_rate
 *
 *	set the mana heal rate on a room
 ***************************************************************************/
static bool set_room_mana_rate(struct char_data *ch, struct roomtemplate *room, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: mana_rate <rate number>\n\r", ch);
        return false;
    }

    set_integer_arg(&room->mana_rate, argument);
    room->mana_rate = UMAX(room->mana_rate, 0);
    return true;
}

/***************************************************************************
 *	set_room_heal_rate
 *
 *	set the hp heal rate on a room
 ***************************************************************************/
static bool set_room_heal_rate(struct char_data *ch, struct roomtemplate *room, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: heal_rate <rate number>\n\r", ch);
        return false;
    }

    set_integer_arg(&room->heal_rate, argument);
    room->heal_rate = UMAX(room->heal_rate, 0);
    return true;
}


/***************************************************************************
 *	set_room_light
 *
 *	set the light value of a room
 ***************************************************************************/
static bool set_room_light(struct char_data *ch, struct roomtemplate *room, const char *argument)
{
    if (is_help(argument)) {
        send_to_char("`#SYNTAX``: light <light duration>\n\r", ch);
        return false;
    }

    set_integer_arg(&room->light, argument);
    room->light = UMAX(room->light, -1);
    return true;
}


/***************************************************************************
 *	skills
 ***************************************************************************/
/***************************************************************************
 *	set_skill
 *
 *	set a characters learend skill information
 ***************************************************************************/
static void set_skill(struct char_data *ch, const char *argument)
{
    struct char_data *vch;
    struct dynamic_skill *skill;
    struct learned_info *learned;
    char arg[MAX_INPUT_LENGTH];
    int percent;

    DENY_NPC(ch);

    if (is_help(argument)) {
        send_to_char("`#SYNTAX``:\n\r", ch);
        send_to_char("  set skill <character name> <spell or skill name> <value>\n\r", ch);
        send_to_char("  set skill <character name> all <value>\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg);

    if ((vch = get_char_world(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC(vch)) {
        send_to_char("Not on NPC's.\n\r", ch);
        return;
    }


    argument = one_argument(argument, arg);
    if (!is_number(argument)) {
        send_to_char("Value must be numeric.\n\r", ch);
        return;
    }

    percent = parse_int(argument);
    if (percent < 0 || percent > 100) {
        send_to_char("Value range is 0 to 100.\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "all")) {
        for (skill = skill_list; skill != NULL; skill = skill->next) {
            learned = new_learned();
            learned->skill = skill;
            learned->percent = percent;
            learned->type = LEARNED_TYPE_SKILL;

            add_learned_skill(vch, learned);
        }
    } else {
        if ((skill = skill_lookup(arg)) == NULL) {
            send_to_char("No such skill or spell.\n\r", ch);
            return;
        }

        if ((learned = get_learned_skill(vch, skill)) != NULL) {
            learned->percent = percent;
        } else {
            learned = new_learned();
            learned->skill = skill;
            learned->percent = percent;
            learned->type = LEARNED_TYPE_SKILL;

            add_learned_skill(vch, learned);
        }
    }
}



/***************************************************************************
 *	set_reboot
 *
 *	initiate or cancel a reboot sequence
 ***************************************************************************/
static void set_reboot(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    int num_ticks;

    if (get_trust(ch) < MAX_LEVEL) {
        send_to_char("Not at your level.\n\r", ch);
        return;
    }

    one_argument(argument, arg);
    if (is_number(arg)) {
        num_ticks = parse_int(arg);

        if (num_ticks > 0) {
            if ((ch->invis_level == ch->level) || (ch->incog_level == ch->level))
                sprintf(buf, " `!R`1eboot sequence initiated by `8someone``.");
            else
                sprintf(buf, " `!R`1eboot sequence initiated by `&%s``.", ch->name);

            do_echo(NULL, buf);

            sprintf(buf, " Reboot in %d ticks.", num_ticks);
            do_echo(NULL, buf);

            globalSystemState.reboot_tick_counter = num_ticks;
        } else {
            if ((ch->invis_level == ch->level) || (ch->incog_level == ch->level))
                sprintf(buf, " Reboot sequence terminated by someone.");
            else
                sprintf(buf, " Reboot sequence terminated by %s.", ch->name);

            do_echo(NULL, buf);

            globalSystemState.reboot_tick_counter = -1;
        }
    } else {
        send_to_char("Value must be numeric.\n\r", ch);
    }
}



/***************************************************************************
 *	set_copyover
 *	shamelessly copied from the above by Dalamar
 *	initiate or cancel a copyover sequence
 ***************************************************************************/
static void set_copyover(struct char_data *ch, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    int num_ticks;

    if (get_trust(ch) < MAX_LEVEL) {
        send_to_char("Not at your level.\n\r", ch);
        return;
    }

    one_argument(argument, arg);
    if (is_number(arg)) {
        num_ticks = parse_int(arg);

        if (num_ticks > 0) {
            if ((ch->invis_level == ch->level) || (ch->incog_level == ch->level))
                sprintf(buf, " `!C`1opyover sequence initiated by `8someone``.");
            else
                sprintf(buf, " `!C`1opyover sequence initiated by `&%s``.", ch->name);

            do_echo(NULL, buf);

            sprintf(buf, " Copyover in %d ticks.", num_ticks);
            do_echo(NULL, buf);

            globalSystemState.copyover_tick_counter = num_ticks;
        } else {
            if ((ch->invis_level == ch->level) || (ch->incog_level == ch->level))
                sprintf(buf, " Copyover sequence terminated by someone.");
            else
                sprintf(buf, " Copyover sequence terminated by %s.", ch->name);

            do_echo(NULL, buf);

            globalSystemState.copyover_tick_counter = -1;
        }
    } else {
        send_to_char("Value must be numeric.\n\r", ch);
    }
}




/***************************************************************************
 *	set function utilities
 ***************************************************************************/
/***************************************************************************
 *	set_integer_arg
 *
 *	set a integer number argument
 ***************************************************************************/
static bool set_integer_arg(int *value, const char *argument)
{
    switch (argument[0]) {
      case '+':
          argument++;
          *value += parse_int(argument);
          return true;
      case '-':
          argument++;
          *value -= parse_int(argument);
          return true;
      default:
          if (is_number(argument)) {
              *value = parse_int(argument);
              return true;
          }
    }

    return false;
}

static bool set_uint_arg(unsigned int *value, const char *argument)
{
    switch (argument[0]) {
      case '+':
          argument++;
          *value += parse_long(argument);
          return true;
      case '-':
          argument++;
          *value -= parse_long(argument);
          return true;
      default:
          if (is_number(argument)) {
              *value = parse_unsigned_int(argument);
              return true;
          }
    }

    return false;
}



/***************************************************************************
 *	set_long_arg
 *
 *	set a long number argument
 ***************************************************************************/
static bool set_long_arg(long *value, const char *argument)
{
    switch (argument[0]) {
      case '+':
          argument++;
          *value += parse_long(argument);
          return true;

      case '-':
          argument++;
          *value -= parse_long(argument);
          return true;

      default:
          if (is_number(argument)) {
              *value = parse_long(argument);
              return true;
          } else {
              return false;
          }
    }
}


/*****************************************************************************
 *	set_obj_value_idx
 *
 *	set an indexed value property of an item based on it's type
 *****************************************************************************/
static bool set_obj_value_idx(struct gameobject *obj, int idx, const char *argument)
{
    struct dynamic_skill *skill;
    int value;

    if (is_number(argument)) {
        obj->value[idx] = parse_int(argument);
        return true;
    } else {
        switch (OBJECT_TYPE(obj)) {
          default:
              break;
          case ITEM_WAND:
          case ITEM_STAFF:
              switch (idx) {
                default:
                    break;
                case 3:
                    if ((skill = skill_lookup(argument)) != NULL) {
                        obj->value[3] = skill->sn;
                        return true;
                    }
              }
              break;

          case ITEM_SCROLL:
          case ITEM_POTION:
          case ITEM_PILL:
              switch (idx) {
                default:
                    break;
                case 1:
                    if ((skill = skill_lookup(argument)) != NULL) {
                        obj->value[1] = skill->sn;
                        return true;
                    }
                    break;
                case 2:
                    if ((skill = skill_lookup(argument)) != NULL) {
                        obj->value[2] = skill->sn;
                        return true;
                    }
                    break;
                case 3:
                    if ((skill = skill_lookup(argument)) != NULL) {
                        obj->value[3] = skill->sn;
                        return true;
                    }
                    break;
                case 4:
                    if ((skill = skill_lookup(argument)) != NULL) {
                        obj->value[4] = skill->sn;
                        return true;
                    }
                    break;
              }
              break;
          case ITEM_WEAPON:
              switch (idx) {
                default:
                    break;
                case 0:
                    ALT_FLAGVALUE_SET(obj->value[0], weapon_class, argument);
                    return true;
                case 3:
                    obj->value[3] = attack_lookup(argument);
                    return true;
                case 4:
                    ALT_FLAGVALUE_TOGGLE(obj->value[4], weapon_flag_type, argument);
                    return true;
              }
              break;

          case ITEM_PORTAL:
              switch (idx) {
                default:
                    break;
                case 1:
                    ALT_FLAGVALUE_SET(obj->value[1], exit_flags, argument);
                    return true;
                case 2:
                    ALT_FLAGVALUE_SET(obj->value[2], portal_flags, argument);
                    return true;
              }
              break;
          case ITEM_FURNITURE:
              switch (idx) {
                default:
                    break;
                case 2:
                    ALT_FLAGVALUE_TOGGLE(obj->value[2], furniture_flags, argument);
                    return true;
              }
              break;
          case ITEM_CONTAINER:
              switch (idx) {
                default:
                    break;
                case 1:
                    if ((value = flag_value(container_flags, argument)) != NO_FLAG) {
                        TOGGLE_BIT(obj->value[1], value);
                        return true;
                    }
                    break;
              }
              break;

          case ITEM_DRINK_CON:
          case ITEM_FOUNTAIN:
              switch (idx) {
                default:
                    break;
                case 2:
                    if ((value = liq_lookup(argument)) >= 0) {
                        obj->value[2] = value;
                        return true;
                    }
                    break;
              }
              break;
        }
    }

    return false;
}



/*****************************************************************************
 *	item_type_help
 *
 *	get help for an item type
 *****************************************************************************/
static void item_type_help(struct char_data *ch, int item_type)
{
    int idx;

    for (idx = 0; item_table[idx].type != 0; idx++) {
        if (item_table[idx].type == item_type) {
            if (item_table[idx].help_keyword[0] != '\0') {
                show_help(ch->desc, item_table[idx].help_keyword, NULL);
                return;
            }
        }
    }
}
