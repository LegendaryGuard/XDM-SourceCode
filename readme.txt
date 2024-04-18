X-Half-Life: Deathmatch version 3.0.3.8 source code

*** Things to remember:
NOTE: please open XHL.sln, not DLL projects. It can also be imported into Code::Blocks.
NOTE: to exclusively debug client DLL and local ("listen") server, set cl_dll as "StartUp Project", to debug HLDS, do it to XDM.
NOTE: read and use comments.txt
WARNING: projects require environment variable: %HLROOT% = C:\games\Half-Life
WARNING: projects are configured to put temporary files onto separate drive (R:)
WARNING: makefiles may be outdated, always keep track of your project DEFINEs.
IMPORTANT: be VERY careful with preprocessor definitions!
NOTE: XHL has much more use of OOP than HL does.
NOTE: GCC just loves conflicting file names. That's why weapons source files are renamed.

*** Things to look at:
console with "developer 2"
output window "Debug"
stdout, stderr (dedicated server mostly)

*** Things to look for:
exception, warning, error, assert, fail, fatal, cannot, unable
(case insensitive)


*** Fixed since last release:

Ammo, items, weapons were not respawning correctly when mp_defaultitems is empty.
Important/quest items were removed when player was disintegrated, telefragged or disconnected.
Weapon box counted weapons even if they were not unpacked to player.
Weapon box now does not store ammo names, only indexes - solves string reallocation problem.
Ammo of exhaustible weapons was added while weapons were not.
Ammo types are now saved with game and merged when ammotypes.txt is changed. (Untested)
Some effects were not drawn parallel to surfaces as intended (SQB).
RPG rocket wrong angles and taret loss (SQB).
Bots angles (pitch) when aiming or following (SQB).
"too many particles" error hidden to level 3
"GetNextBestItem(%d) == WEAPON_NONE!" warning that was caused by bad weapon status on server.
Entities no longer respawn after being deleted by action command.
Award name localization in statistics window.
Some projectiles now produce sound when colliding with water.
trigger_teleport endless loop when target is changed externally.
env_flamespawner created clouds with yellow particles for no reason.
gibs placed with "body" < 0 would disappear.

(remove and replace this section when changing build version)



- Xawari (c) 2000-2017

License:
VALVe-derived license for valve-produced files.
FMOD library is distributed under Firelight Technologies Pty, Ltd own license.
SDL library is distributed under the zlib license.
Files produced by Xawari are distributed under the MPL license version 2.0.
Other files, libraries and possibly parts of code are subjects of copyright of their respective owners.
Premission for redistribution and/or modification for commercial purposes should be requested separately in written form.
