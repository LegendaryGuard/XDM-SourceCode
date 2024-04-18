# X-Half-Life
# Copyright (c) Xawari
# XHL Linux port
# Copyright (c) mittorn
# NOTE: defines must be taken from project file

CC=gcc
CXX=g++

PROJECTDEFINES = -fexceptions -std=gnu++98 -fsigned-char -fpermissive -Wextra -Wno-write-strings \
    -DLINUX -D_LINUX -DCLIENT_DLL -DNOSQB -DUSE_EXCEPTIONS -DNO_VOICEGAMEMGR -DUSE_WEAPONBITS -DOLD_DEATHMSG -DSV_NO_PITCH_CORRECTION -DCLDLL_FIX_PLAYER_ATTACHMENTS \
    -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_strtime=strtime -Wno-invalid-offsetof -Wno-conversion-null

ifeq ($(DEBUG),1)
BUILDCONFIGNAME = debug
BUILDCONFIGFLAGS = -O0 -g -fno-omit-frame-pointer -D_DEBUG
else
BUILDCONFIGNAME = release
BUILDCONFIGFLAGS = -O2 -frtti -DNDEBUG
endif

TARGET = client.so
TARGETDIR = $(BUILDCONFIGNAME)_linux
TARGETPATH = $(TARGETDIR)/$(TARGET)
INTERMEDIATE_PATH = $(TARGETDIR)/intermediate

SRCS :=
SRCS += ../common/util_common.cpp
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
SRCS += ../game_shared/voice_vgui_tweakdlg.cpp
#SRCS += ../pm_shared/pm_debug.c
#SRCS += ../pm_shared/pm_math.c
SRCS += ../pm_shared/pm_shared.cpp
SRCS += ../cl_dll/ev_common.cpp
SRCS += ../cl_dll/ev_fx.cpp
SRCS += ../cl_dll/ev_game.cpp
SRCS += ../cl_dll/ev_weapons.cpp
SRCS += ../cl_dll/events.cpp
SRCS += ../cl_dll/hud_ammo.cpp
SRCS += ../cl_dll/hud_historyresource.cpp
SRCS += ../cl_dll/hud_battery.cpp
SRCS += ../cl_dll/hud_deathnotice.cpp
SRCS += ../cl_dll/hud_flashlight.cpp
SRCS += ../cl_dll/hud_geiger.cpp
SRCS += ../cl_dll/hud_health.cpp
SRCS += ../cl_dll/hud.cpp
SRCS += ../cl_dll/hud_bench.cpp
SRCS += ../cl_dll/hud_benchtrace.cpp
SRCS += ../cl_dll/hud_domdisplay.cpp
SRCS += ../cl_dll/hud_flagdisplay.cpp
SRCS += ../cl_dll/hud_gamedisplay.cpp
SRCS += ../cl_dll/hud_gamerules.cpp
SRCS += ../cl_dll/hud_msg.cpp
SRCS += ../cl_dll/hud_redraw.cpp
SRCS += ../cl_dll/hud_rocketscreen.cpp
SRCS += ../cl_dll/hud_servers.cpp
SRCS += ../cl_dll/hud_spectator.cpp
SRCS += ../cl_dll/hud_update.cpp
SRCS += ../cl_dll/hud_weaponresource.cpp
SRCS += ../cl_dll/hud_zoomcrosshair.cpp
SRCS += ../cl_dll/hud_message.cpp
SRCS += ../cl_dll/hud_saytext.cpp
SRCS += ../cl_dll/hud_status_icons.cpp
SRCS += ../cl_dll/hud_statusbar.cpp
SRCS += ../cl_dll/hud_timer.cpp
SRCS += ../cl_dll/hud_train.cpp
SRCS += ../cl_dll/bsputil.cpp
SRCS += ../cl_dll/cdll_int.cpp
SRCS += ../cl_dll/cl_cmd.cpp
SRCS += ../cl_dll/cl_fx.cpp
SRCS += ../cl_dll/cl_msg.cpp
SRCS += ../cl_dll/cl_msg_fx.cpp
SRCS += ../cl_dll/com_weapons.cpp
SRCS += ../cl_dll/demo.cpp
SRCS += ../cl_dll/entity.cpp
SRCS += ../cl_dll/in_camera.cpp
SRCS += ../cl_dll/input.cpp
SRCS += ../cl_dll/inputw32.cpp
SRCS += ../cl_dll/interpolation.cpp
SRCS += ../cl_dll/musicplayer.cpp
SRCS += ../cl_dll/parsemsg.cpp
SRCS += ../cl_dll/util.cpp
SRCS += ../cl_dll/view.cpp
SRCS += ../cl_dll/GameStudioModelRenderer.cpp
SRCS += ../cl_dll/studio_util.cpp
SRCS += ../cl_dll/StudioModelRenderer.cpp
SRCS += ../cl_dll/tri.cpp
SRCS += ../cl_dll/vgui_CommandMenu.cpp
SRCS += ../cl_dll/vgui_CustomObjects.cpp
SRCS += ../cl_dll/vgui_EntityEntryPanel.cpp
SRCS += ../cl_dll/vgui_Int.cpp
SRCS += ../cl_dll/vgui_MenuPanel.cpp
SRCS += ../cl_dll/vgui_MessageWindow.cpp
SRCS += ../cl_dll/vgui_MusicPlayer.cpp
SRCS += ../cl_dll/vgui_PSEditorPanel.cpp
SRCS += ../cl_dll/vgui_SchemeManager.cpp
SRCS += ../cl_dll/vgui_ScorePanel.cpp
SRCS += ../cl_dll/vgui_ServerBrowser.cpp
SRCS += ../cl_dll/vgui_SpectatorPanel.cpp
SRCS += ../cl_dll/vgui_StatsPanel.cpp
SRCS += ../cl_dll/vgui_TeamMenu.cpp
SRCS += ../cl_dll/vgui_Viewport.cpp
SRCS += ../cl_dll/voice_status.cpp
SRCS += ../cl_dll/hl/hl_baseentity.cpp
SRCS += ../cl_dll/hl/hl_weapons.cpp
SRCS += ../cl_dll/rendersystem/Particle.cpp
SRCS += ../cl_dll/rendersystem/ParticleSystem.cpp
SRCS += ../cl_dll/rendersystem/PSBeam.cpp
SRCS += ../cl_dll/rendersystem/PSBubbles.cpp
SRCS += ../cl_dll/rendersystem/PSCustom.cpp
SRCS += ../cl_dll/rendersystem/PSDrips.cpp
SRCS += ../cl_dll/rendersystem/PSFlameCone.cpp
SRCS += ../cl_dll/rendersystem/PSFlatTrail.cpp
SRCS += ../cl_dll/rendersystem/RSRadialBeams.cpp
SRCS += ../cl_dll/rendersystem/PSSparks.cpp
SRCS += ../cl_dll/rendersystem/PSSpiralEffect.cpp
SRCS += ../cl_dll/rendersystem/RenderManager.cpp
SRCS += ../cl_dll/rendersystem/RenderSystem.cpp
SRCS += ../cl_dll/rendersystem/RotatingSystem.cpp
SRCS += ../cl_dll/rendersystem/RSBeam.cpp
SRCS += ../cl_dll/rendersystem/RSBeamStar.cpp
SRCS += ../cl_dll/rendersystem/RSCylinder.cpp
SRCS += ../cl_dll/rendersystem/RSDelayed.cpp
SRCS += ../cl_dll/rendersystem/RSDisk.cpp
SRCS += ../cl_dll/rendersystem/RSLight.cpp
SRCS += ../cl_dll/rendersystem/RSModel.cpp
SRCS += ../cl_dll/rendersystem/RSSphere.cpp
SRCS += ../cl_dll/rendersystem/RSSprite.cpp
SRCS += ../cl_dll/rendersystem/RSTeleparts.cpp
SRCS += ../game_shared/vgui_checkbutton2.cpp
SRCS += ../game_shared/vgui_grid.cpp
SRCS += ../game_shared/vgui_listbox.cpp
SRCS += ../game_shared/vgui_loadtga.cpp
SRCS += ../game_shared/vgui_scrollbar2.cpp
SRCS += ../game_shared/vgui_slider2.cpp
SRCS += ../common/color.cpp
SRCS += ../common/materials.cpp
SRCS += ../common/shared_resources.cpp
SRCS += ../common/util_vector.cpp
SRCS += ../common/vector.cpp
SRCS += ../public/interface.cpp


INCLUDES = -I. -Iwpn_shared -I../common -I../engine/common \
    -I../engine -I../public -I../pm_shared -I../game_shared \
    -I../dlls/gamerules -I../dlls/projectiles -I../game_shared/weapons \
    -I../dlls -I../utils/vgui/include -Irendersystem -I../external

OBJS = $(patsubst ../%.cpp,$(INTERMEDIATE_PATH)/%.o,$(SRCS)) $(patsubst ../%.c,$(INTERMEDIATE_PATH)/%.o,$(SRCS_C))

DIRS = $(INTERMEDIATE_PATH) $(INTERMEDIATE_PATH)/common $(INTERMEDIATE_PATH)/dlls $(INTERMEDIATE_PATH)/dlls/gamerules $(INTERMEDIATE_PATH)/dlls/projectiles $(INTERMEDIATE_PATH)/game_shared/weapons $(INTERMEDIATE_PATH)/pm_shared $(INTERMEDIATE_PATH)/dlls/monsters $(INTERMEDIATE_PATH)/cl_dll/rendersystem $(INTERMEDIATE_PATH)/cl_dll/hl $(INTERMEDIATE_PATH)/public

CFLAGS = -m32 -march=i686 -mtune=generic $(BUILDCONFIGFLAGS)
LDFLAGS = -m32 -lstdc++ -static-libgcc

client.so : $(OBJS) vgui.so libSDL2-2.0.so.0
	$(CXX) $(LDFLAGS) $(OBJS) -o $(BUILDCONFIGNAME)_linux/client.so -shared -Wl,--no-undefined -fPIC -lm -ldl vgui.so libSDL2-2.0.so.0


$(OBJS) :| $(DIRS)

$(DIRS):
	mkdir -p $@

$(INTERMEDIATE_PATH)/%.o : ../%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(PROJECTDEFINES) -fPIC -c $< -o $@

$(INTERMEDIATE_PATH)/%.o : ../%.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) $(PROJECTDEFINES) -fPIC -c $< -o $@


vgui.so: ../utils/vgui/lib/linux/vgui.so
	cp $< $@

libSDL2-2.0.so.0: ../lib/public/libSDL2-2.0.so.0
	cp $< $@

clean:
	$(RM) $(OBJS) libSDL2-2.0.so.0 vgui.so $(BUILDCONFIGNAME)_linux/client.so
