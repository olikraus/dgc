# required subdirs:
# app
# cube
# util 
# cfg: contains only config.h
# gnet: only dgd_opt.h
# gnet & syl only required for dgc

CFLAGS = -g -Wall -ffunction-sections -fdata-sections -I. -Icube -Iutil -Iencoding -Ignet -Isyl -DMWC_DISABLE
LDFLAGS = -Wl,--gc-sections


SRC = $(shell ls cube/*.c) $(shell ls util/*.c) $(shell ls encoding/*.c) $(shell ls gnet/*.c) $(shell ls syl/*.c) 

OBJ = $(SRC:.c=.o)

all: dgsop simfsm xbm2pla bms2kiss dgc dglc pluc

dgsop: $(OBJ) ./app/dgsop.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) ./app/dgsop.o -o dgsop -lm

simfsm: $(OBJ) ./app/simfsm.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) ./app/simfsm.o -o simfsm -lm

xbm2pla: $(OBJ) ./app/xbm2pla.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) ./app/xbm2pla.o -o xbm2pla -lm

bms2kiss: $(OBJ) ./app/bms2kiss.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) ./app/bms2kiss.o -o bms2kiss -lm

dgc: $(OBJ) ./app/dgc.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) ./app/dgc.o -o dgc -lm

dglc: $(OBJ) ./app/dglc.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) ./app/dglc.o -o dglc -lm
	
pluc: $(OBJ) ./app/pluc.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) ./app/pluc.o -o pluc -lm

clean:
	-rm $(OBJ) ./app/*.o  dgsop simfsm xbm2pla bms2kiss dgc dglc
	