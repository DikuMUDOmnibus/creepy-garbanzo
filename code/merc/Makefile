IDIR=../include
ODIR=../obj/merc
BDIR=../bin
CC= /usr/bin/gcc
C_FLAGS_DEBUG= -g2 -Wall -pedantic
C_FLAGS_RELEASE= -Ofast
C_FLAGS= $(C_FLAGS_DEBUG) -std=gnu99 -I$(IDIR)
L_FLAGS= -DLINUX
LIBS=-lm

SDIR=.
_INCLUDES=merc.h sysinternals.h
DEPS=$(patsubst %,$(IDIR)/%,$(_INCLUDES)) $(SYSINTERNALS)
MERC=$(BDIR)/merc.a
_OBJ=act_comm.o act_combat.o act_enter.o act_info.o act_move.o act_obj.o act_wiz.o \
     act.o affects.o alias.o at.o auction.o \
     ban.o bank.o bit.o \
     channels.o character.o character_service.o combat_roll.o comm.o const.o copyover.o \
     db.o descriptor.o \
     effects.o \
     fight.o find.o flags.o follower_service.o for.o \
     handler.o healer.o help.o \
     interp.o \
     libfile.o lookup.o \
     magic.o mem.o mob_cmds.o mob_prog.o mobs.o \
     nanny.o note.o \
     object.o ovnum.o \
     password.o \
     race.o recycle.o remote.o \
     save.o scan.o serialize.o set.o skills.o spells.o stat.o \
     tables.o teleport.o \
     update.o variant.o who.o \
     elements/affectdata.o elements/area.o \
     elements/exittemplate.o elements/extradescrdata.o \
     elements/objecttemplate.o elements/resettemplate.o elements/roomtemplate.o \
     olc/olc.o olc/olc_areas.o olc/olc_groups.o olc/olc_helps.o olc/olc_mobs.o \
     olc/olc_objects.o olc/olc_progs.o olc/olc_rooms.o olc/olc_save.o olc/olc_skills.o  \
     olc/olc_string_editor.o


OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))
SYSINTERNALS=$(BDIR)/sysinternals.a


.PHONY: clean all install

all: $(MERC)

prebuild:
	splint -DLINUX -I$(IDIR) -exportlocal elements/objecttemplate.c descriptor.c object.c help.c elements/area.c elements/affectdata.c elements/extradescrdata.c olc/olc_string_editor.c

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
	mkdir $(ODIR)/elements 2>/dev/null

clean:
	rm -f $(ODIR)/*.o $(ODIR)/olc/*.o $(MERC)

