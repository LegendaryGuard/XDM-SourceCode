# X-Half-Life GAME DLL
# Copyright (c) Xawari
# XHL Linux port
# Copyright (c) mittorn
# NOTE: defines must be taken from project file

CC=gcc
CXX=g++

PROJECTDEFINES = -fexceptions -std=gnu++98 -fsigned-char -fpermissive -Wextra -Wno-write-strings \
    -DLINUX -D_LINUX -DSERVER_DLL -DNOSQB -DUSE_EXCEPTIONS -DNO_VOICEGAMEMGR -DUSE_WEAPONBITS -DOLD_DEATHMSG -DSV_NO_PITCH_CORRECTION -DSVDLL_NEWFUNCTIONS \
    -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_strtime=strtime -Wno-invalid-offsetof -Wno-conversion-null

ifeq ($(DEBUG),1)
BUILDCONFIGNAME = debug
BUILDCONFIGFLAGS = -O0 -g -fno-omit-frame-pointer -D_DEBUG
else
BUILDCONFIGNAME = release
BUILDCONFIGFLAGS = -O2 -frtti -DNDEBUG
endif

TARGET = XDM.so
TARGETDIR = $(BUILDCONFIGNAME)_linux
TARGETPATH = $(TARGETDIR)/$(TARGET)
INTERMEDIATE_PATH = $(TARGETDIR)/intermediate

SRCS :=
SRCS += ../common/color.cpp
SRCS += ../common/materials.cpp
SRCS += ../common/shared_resources.cpp
SRCS += ../common/util_common.cpp
SRCS += ../common/util_vector.cpp
SRCS += ../common/vector.cpp
SRCS += ../game_shared/weapons/WeaponAcidLauncher.cpp
SRCS += ../game_shared/weapons/WeaponBeamRifle.cpp
SRCS += ../game_shared/weapons/WeaponBHG.cpp
SRCS += ../game_shared/weapons/WeaponChemGun.cpp
SRCS += ../game_shared/weapons/WeaponCrossbow.cpp
SRCS += ../game_shared/weapons/WeaponCrowbar.cpp
SRCS += ../game_shared/weapons/WeaponCustom.cpp
SRCS += ../game_shared/weapons/WeaponDiskLauncher.cpp
SRCS += ../game_shared/weapons/WeaponDisplacer.cpp
SRCS += ../game_shared/weapons/WeaponEgon.cpp
SRCS += ../game_shared/weapons/WeaponFlame.cpp
SRCS += ../game_shared/weapons/WeaponGauss.cpp
SRCS += ../game_shared/weapons/WeaponGlock.cpp
SRCS += ../game_shared/weapons/WeaponGrenadeLauncher.cpp
SRCS += ../game_shared/weapons/WeaponHandGrenade.cpp
SRCS += ../game_shared/weapons/WeaponHornetGun.cpp
SRCS += ../game_shared/weapons/WeaponMP5.cpp
SRCS += ../game_shared/weapons/WeaponPlasma.cpp
SRCS += ../game_shared/weapons/WeaponPython.cpp
SRCS += ../game_shared/weapons/WeaponRPG.cpp
SRCS += ../game_shared/weapons/weapons.cpp
SRCS += ../game_shared/weapons/WeaponSatchel.cpp
SRCS += ../game_shared/weapons/WeaponShotgun.cpp
SRCS += ../game_shared/weapons/WeaponSniperRifle.cpp
SRCS += ../game_shared/weapons/WeaponSqueak.cpp
SRCS += ../game_shared/weapons/WeaponStrikeTarget.cpp
SRCS += ../game_shared/weapons/WeaponSword.cpp
SRCS += ../game_shared/weapons/WeaponTripmine.cpp
SRCS += ../game_shared/voice_banmgr.cpp
SRCS += ../game_shared/voice_gamemgr.cpp
#SRCS += ../pm_shared/pm_debug.c
#SRCS += ../pm_shared/pm_math.c
SRCS += ../pm_shared/pm_shared.cpp
SRCS += ../dlls/monsters/ai_basenpc_schedule.cpp
SRCS += ../dlls/monsters/aflock.cpp
SRCS += ../dlls/monsters/agrunt.cpp
SRCS += ../dlls/monsters/apache.cpp
SRCS += ../dlls/monsters/barnacle.cpp
SRCS += ../dlls/monsters/barney.cpp
SRCS += ../dlls/monsters/bigmomma.cpp
SRCS += ../dlls/monsters/bloater.cpp
SRCS += ../dlls/monsters/bullsquid.cpp
SRCS += ../dlls/monsters/controller.cpp
SRCS += ../dlls/monsters/flyingmonster.cpp
SRCS += ../dlls/monsters/gargantua.cpp
SRCS += ../dlls/monsters/genericmonster.cpp
SRCS += ../dlls/monsters/gman.cpp
SRCS += ../dlls/monsters/hassassin.cpp
SRCS += ../dlls/monsters/headcrab.cpp
SRCS += ../dlls/monsters/hgrunt.cpp
SRCS += ../dlls/monsters/houndeye.cpp
SRCS += ../dlls/monsters/ichthyosaur.cpp
SRCS += ../dlls/monsters/islave.cpp
SRCS += ../dlls/monsters/leech.cpp
SRCS += ../dlls/monsters/massassin.cpp
SRCS += ../dlls/monsters/monsters.cpp
SRCS += ../dlls/monsters/monsterstate.cpp
SRCS += ../dlls/monsters/nihilanth.cpp
SRCS += ../dlls/monsters/osprey.cpp
SRCS += ../dlls/monsters/pitdrone.cpp
SRCS += ../dlls/monsters/playermonster.cpp
SRCS += ../dlls/monsters/rat.cpp
SRCS += ../dlls/monsters/roach.cpp
SRCS += ../dlls/monsters/scientist.cpp
SRCS += ../dlls/monsters/shocktrooper.cpp
SRCS += ../dlls/monsters/squadmonster.cpp
SRCS += ../dlls/monsters/talkmonster.cpp
SRCS += ../dlls/monsters/tentacle.cpp
SRCS += ../dlls/monsters/turret.cpp
SRCS += ../dlls/monsters/voltigore.cpp
SRCS += ../dlls/monsters/zombie.cpp
SRCS += ../dlls/gamerules/coop_gamerules.cpp
SRCS += ../dlls/gamerules/ctf_gamerules.cpp
SRCS += ../dlls/gamerules/dom_gamerules.cpp
SRCS += ../dlls/gamerules/gamerules.cpp
SRCS += ../dlls/gamerules/lms_gamerules.cpp
SRCS += ../dlls/gamerules/maprules.cpp
SRCS += ../dlls/gamerules/multiplay_gamerules.cpp
SRCS += ../dlls/gamerules/round_gamerules.cpp
SRCS += ../dlls/gamerules/singleplay_gamerules.cpp
SRCS += ../dlls/gamerules/teamplay_gamerules.cpp
SRCS += ../dlls/animating.cpp
SRCS += ../dlls/bmodels.cpp
SRCS += ../dlls/buttons.cpp
SRCS += ../dlls/doors.cpp
SRCS += ../dlls/effects.cpp
SRCS += ../dlls/effects_new.cpp
SRCS += ../dlls/ent_alias.cpp
SRCS += ../dlls/ent_functional.cpp
SRCS += ../dlls/ent_locus.cpp
SRCS += ../dlls/environment.cpp
SRCS += ../dlls/explode.cpp
SRCS += ../dlls/func_break.cpp
SRCS += ../dlls/func_tank.cpp
SRCS += ../dlls/gibs.cpp
SRCS += ../dlls/h_cycler.cpp
SRCS += ../dlls/items.cpp
SRCS += ../dlls/lights.cpp
SRCS += ../dlls/monstermaker.cpp
SRCS += ../dlls/mortar.cpp
SRCS += ../dlls/pathcorner.cpp
SRCS += ../dlls/plats.cpp
SRCS += ../dlls/scripted.cpp
SRCS += ../dlls/sound.cpp
SRCS += ../dlls/soundent.cpp
SRCS += ../dlls/subs.cpp
SRCS += ../dlls/triggers.cpp
SRCS += ../dlls/triggers_new.cpp
SRCS += ../dlls/wallcharger.cpp
SRCS += ../dlls/weaponbox.cpp
SRCS += ../dlls/xen.cpp
SRCS += ../dlls/ammo.cpp
SRCS += ../dlls/animation.cpp
SRCS += ../dlls/cbase.cpp
SRCS += ../dlls/client.cpp
SRCS += ../dlls/combat.cpp
SRCS += ../dlls/defaultai.cpp
SRCS += ../dlls/entconfig.cpp
SRCS += ../dlls/game.cpp
SRCS += ../dlls/globals.cpp
SRCS += ../dlls/h_ai.cpp
SRCS += ../dlls/h_export.cpp
SRCS += ../dlls/mapcycle.cpp
SRCS += ../dlls/movewith.cpp
SRCS += ../dlls/nodes.cpp
SRCS += ../dlls/observer.cpp
SRCS += ../dlls/plane.cpp
SRCS += ../dlls/player.cpp
SRCS += ../dlls/saverestore.cpp
SRCS += ../dlls/skill.cpp
SRCS += ../dlls/spectator.cpp
SRCS += ../dlls/util.cpp
SRCS += ../dlls/util_xhl.cpp
SRCS += ../dlls/world.cpp
SRCS += ../dlls/projectiles/agrenade.cpp
SRCS += ../dlls/projectiles/argrenade.cpp
SRCS += ../dlls/projectiles/blackball.cpp
SRCS += ../dlls/projectiles/crossbowbolt.cpp
SRCS += ../dlls/projectiles/customprojectile.cpp
SRCS += ../dlls/projectiles/flamecloud.cpp
SRCS += ../dlls/projectiles/ggrenade.cpp
SRCS += ../dlls/projectiles/hornet.cpp
SRCS += ../dlls/projectiles/lgrenade.cpp
SRCS += ../dlls/projectiles/lightp.cpp
SRCS += ../dlls/projectiles/mortartarget.cpp
SRCS += ../dlls/projectiles/nucleardevice.cpp
SRCS += ../dlls/projectiles/plasmaball.cpp
SRCS += ../dlls/projectiles/projectile.cpp
SRCS += ../dlls/projectiles/razordisk.cpp
SRCS += ../dlls/projectiles/rpgrocket.cpp
SRCS += ../dlls/projectiles/satchelcharge.cpp
SRCS += ../dlls/projectiles/squeakgrenade.cpp
SRCS += ../dlls/projectiles/teleporter.cpp
SRCS += ../dlls/projectiles/tripminegrenade.cpp


INCLUDES = -I. -I../common -I../engine -I../engine/common -I../public -I../pm_shared \
    -Igamerules -Iprojectiles -I../game_shared -I../game_shared/weapons

OBJS = $(patsubst ../%.cpp,$(INTERMEDIATE_PATH)/%.o,$(SRCS)) $(patsubst ../%.c,$(INTERMEDIATE_PATH)/%.o,$(SRCS_C))

DIRS = $(INTERMEDIATE_PATH) $(INTERMEDIATE_PATH)/common $(INTERMEDIATE_PATH)/dlls $(INTERMEDIATE_PATH)/dlls/gamerules $(INTERMEDIATE_PATH)/dlls/projectiles $(INTERMEDIATE_PATH)/game_shared/weapons $(INTERMEDIATE_PATH)/pm_shared $(INTERMEDIATE_PATH)/dlls/monsters

CFLAGS = -m32 -march=i686 -mtune=generic $(BUILDCONFIGFLAGS)
LDFLAGS = -m32 -lstdc++ -static-libgcc

$(TARGET) : $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $(TARGETPATH) -shared -Wl,--no-undefined -fPIC -lm -ldl

$(OBJS) :| $(DIRS)

$(DIRS):
	mkdir -p $@

$(INTERMEDIATE_PATH)/%.o : ../%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(PROJECTDEFINES) -fPIC -c $< -o $@

$(INTERMEDIATE_PATH)/%.o : ../%.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) $(PROJECTDEFINES) -fPIC -c $< -o $@

clean:
	$(RM) $(OBJS) $(BUILDCONFIGNAME)_linux/XDM.so
