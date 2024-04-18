ARCH=i486
CPP = g++ -m32
CFLAGS = -march=i686 -mtune=generic
BASEFLAGS = -Dstricmp=strcasecmp -Dstrcmpi=strcasecmp -DSVDLL_NEWFUNCTIONS -DPROXY_GAME_DLL -DSV_NO_PITCH_CORRECTION
CPPFLAGS = ${BASEFLAGS} -w -I../dlls -I../engine -I../common -I../pm_shared -I../public -fpermissive
ifeq ($(DEBUG),1)
TYPE = debug
DEBUGFLAGS =  -O0 -g -fno-omit-frame-pointer  -D_DEBUG
else
TYPE = release
DEBUGFLAGS = -O2
endif
#OBJ = bot.o bot_chat.o bot_client.o bot_combat.o bot_commands.o bot_logo.o bot_navigate.o dll.o engine.o h_export.o linkfunc.o 
#namefunc.o \
#util.o waypoint.o ../common/util_common.o ../common/util_vector.o ../common/vector.o

OBJ = obj/common/util_common.o obj/common/util_vector.o obj/common/vector.o \
obj/xbm/bot.o obj/xbm/bot_chat.o obj/xbm/bot_client.o obj/xbm/bot_combat.o obj/xbm/bot_commands.o obj/xbm/bot_logo.o \
obj/xbm/bot_navigate.o obj/xbm/dll.o obj/xbm/engine.o obj/xbm/h_export.o obj/xbm/linkfunc.o obj/xbm/namefunc.o \
obj/xbm/util.o obj/xbm/waypoint.o 
TOP_DIR := ..

XBM_$(ARCH)_$(TYPE).so: ${OBJ}
	${CPP} -Wl,--no-undefined -lm -shared -o $@ ${OBJ} -Xlinker -Map -Xlinker XBM.map -ldl

DIRS = obj obj/common obj/xbm

${OBJ}: | ${DIRS}

${DIRS}:
	mkdir -p $@

clean:
	-rm -f $(OBJ)
	-rm -f XBM_$(ARCH)_$(TYPE).so

obj/%.o:	${TOP_DIR}/%.cpp
	${CPP} ${CPPFLAGS} ${CFLAGS} ${DEBUGFLAGS} -c $< -o $@

obj/%.o:	${TOP_DIR}/%.c
	${CPP} ${CPPFLAGS} ${CFLAGS} ${DEBUGFLAGS} -c $< -o $@
