IDIR=../include
ODIR=../obj/merc
BDIR=../bin
CC= /usr/bin/gcc
PROF= -DLINUX
C_FLAGS= -O2 -g2 -Wall -pedantic -std=gnu99 -I$(IDIR)
L_FLAGS=  $(PROF)
LIBS=-lm

SDIR=.
_INCLUDES=merc.h sysinternals.h
DEPS=$(patsubst %,$(IDIR)/%,$(_INCLUDES)) $(SYSINTERNALS)
MERC=$(BDIR)/merc.a
_OBJ=act_comm.o act_combat.o act_enter.o act_info.o act_move.o act_obj.o act_wiz.o \
     affects.o alias.o at.o auction.o \
     ban.o bank.o bit.o \
     channels.o character.o character_service.o combat_roll.o comm.o const.o \
     db.o descriptor.o \
     effects.o enchant.o \
     fight.o find.o flags.o follower_service.o for.o \
     handler.o healer.o help.o \
     imprint.o interp.o \
     libfile.o logging.o lookup.o \
     magic.o mem.o mob_cmds.o mob_prog.o mobs.o \
     nanny.o note.o \
     object.o objectprototype.o ovnum.o \
     password.o \
     race.o recycle.o repair.o \
     save.o scan.o set.o skills.o socketio.o spells.o stat.o \
     tables.o teleport.o \
     unique.o update.o variant.o who.o \
     olc/olc.o olc/olc_areas.o olc/olc_groups.o olc/olc_helps.o olc/olc_mobs.o \
     olc/olc_objects.o olc/olc_progs.o olc/olc_rooms.o olc/olc_save.o olc/olc_skills.o  \
     olc/string.o 
	 

OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))
SYSINTERNALS=$(BDIR)/sysinternals.a


.PHONY: clean all install

all: $(MERC)

prebuild:
	splint -DLINUX -I$(IDIR) -exportlocal objectprototype.c descriptor.c object.c logging.c

$(SYSINTERNALS):
	cd ../sysinternals && $(MAKE)

$(MERC): $(OBJ) $(DEPS) prebuild | $(BDIR)
	ar rcs $(MERC) $@ $(OBJ) $(SYSINTERNALS) 

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(C_FLAGS)

$(BDIR):
	mkdir $(BDIR) 2>/dev/null

$(ODIR):
	mkdir $(ODIR) 2>/dev/null
	mkdir $(ODIR)/olc 2>/dev/null

clean:
	rm -f $(ODIR)/*.o $(ODIR)/olc/*.o $(MERC)
