#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "bot.h"

#ifdef _WIN32
extern HINSTANCE h_Library;
#else
extern void *h_Library;
#endif

#define LINK_ENTITY_TO_FUNC(mapClassName)\
extern "C" void DLLEXPORT mapClassName(entvars_t *pev) {\
	static LINK_ENTITY_FUNC otherClassName = NULL;\
	static int skip_this = 0;\
	if (skip_this) return;\
	if (otherClassName == NULL)\
		otherClassName = (LINK_ENTITY_FUNC)GetProcAddress(h_Library, #mapClassName);\
	if (otherClassName == NULL) {\
		printf("XBM: cannot init '%s'\n", #mapClassName);\
		skip_this = 1; return;\
	}\
	(*otherClassName)(pev); }


LINK_ENTITY_TO_FUNC(DelayedUse);
LINK_ENTITY_TO_FUNC(a_grenade);
LINK_ENTITY_TO_FUNC(ar_grenade);
LINK_ENTITY_TO_FUNC(aiscripted_sequence);
LINK_ENTITY_TO_FUNC(ambient_generic);
LINK_ENTITY_TO_FUNC(ammo);
LINK_ENTITY_TO_FUNC(ammo_357);
LINK_ENTITY_TO_FUNC(ammo_9mmAR);
LINK_ENTITY_TO_FUNC(ammo_9mmARclip);
LINK_ENTITY_TO_FUNC(ammo_9mmbox);
LINK_ENTITY_TO_FUNC(ammo_9mmclip);
LINK_ENTITY_TO_FUNC(ammo_ARgrenades);
LINK_ENTITY_TO_FUNC(ammo_alauncher)
LINK_ENTITY_TO_FUNC(ammo_buckshot);
LINK_ENTITY_TO_FUNC(ammo_chemgun);
LINK_ENTITY_TO_FUNC(ammo_crossbow);
LINK_ENTITY_TO_FUNC(ammo_egonclip);
LINK_ENTITY_TO_FUNC(ammo_fueltank);
LINK_ENTITY_TO_FUNC(ammo_gaussclip);
LINK_ENTITY_TO_FUNC(ammo_glauncher);
LINK_ENTITY_TO_FUNC(ammo_glockclip);
LINK_ENTITY_TO_FUNC(ammo_lightp);
LINK_ENTITY_TO_FUNC(ammo_mp5clip);
LINK_ENTITY_TO_FUNC(ammo_mp5grenades);
LINK_ENTITY_TO_FUNC(ammo_razorblades);
LINK_ENTITY_TO_FUNC(ammo_rpgclip);
LINK_ENTITY_TO_FUNC(ammo_sniper);
LINK_ENTITY_TO_FUNC(beam);
LINK_ENTITY_TO_FUNC(blackball);
LINK_ENTITY_TO_FUNC(bmortar);
LINK_ENTITY_TO_FUNC(bodyque);
LINK_ENTITY_TO_FUNC(bolt);
LINK_ENTITY_TO_FUNC(button_target);
LINK_ENTITY_TO_FUNC(calc_position);// SHL compatibility
LINK_ENTITY_TO_FUNC(calc_ratio);// SHL compatibility
LINK_ENTITY_TO_FUNC(calc_subvelocity);// SHL compatibility
LINK_ENTITY_TO_FUNC(calc_velocity_path);// SHL compatibility
LINK_ENTITY_TO_FUNC(calc_velocity_polar);// SHL compatibility
LINK_ENTITY_TO_FUNC(controller_energy_ball);
LINK_ENTITY_TO_FUNC(controller_head_ball);
LINK_ENTITY_TO_FUNC(crossbow_bolt);
LINK_ENTITY_TO_FUNC(cycler);
LINK_ENTITY_TO_FUNC(cycler_prdroid);
LINK_ENTITY_TO_FUNC(cycler_sprite);
LINK_ENTITY_TO_FUNC(cycler_weapon);
LINK_ENTITY_TO_FUNC(cycler_wreckage);
LINK_ENTITY_TO_FUNC(entity_chasecam);
LINK_ENTITY_TO_FUNC(env_ammodispenser);
LINK_ENTITY_TO_FUNC(env_beam);
LINK_ENTITY_TO_FUNC(env_beamtrail);
LINK_ENTITY_TO_FUNC(env_beverage);
LINK_ENTITY_TO_FUNC(env_blood);
LINK_ENTITY_TO_FUNC(env_bubbles);
LINK_ENTITY_TO_FUNC(env_cache);
LINK_ENTITY_TO_FUNC(env_debris);
LINK_ENTITY_TO_FUNC(env_decal);
LINK_ENTITY_TO_FUNC(env_dlight);
LINK_ENTITY_TO_FUNC(env_elight);
LINK_ENTITY_TO_FUNC(env_explosion);
LINK_ENTITY_TO_FUNC(env_fade);
LINK_ENTITY_TO_FUNC(env_flamespawner);
LINK_ENTITY_TO_FUNC(env_fog);
LINK_ENTITY_TO_FUNC(env_fogzone);
LINK_ENTITY_TO_FUNC(env_fountain);
LINK_ENTITY_TO_FUNC(env_funnel);
LINK_ENTITY_TO_FUNC(env_global);
LINK_ENTITY_TO_FUNC(env_glow);
LINK_ENTITY_TO_FUNC(env_gravitypoint);
LINK_ENTITY_TO_FUNC(env_laser);
LINK_ENTITY_TO_FUNC(env_lightatt);
LINK_ENTITY_TO_FUNC(env_lightning);
LINK_ENTITY_TO_FUNC(env_message);
LINK_ENTITY_TO_FUNC(env_model);
LINK_ENTITY_TO_FUNC(env_rendersystem);
LINK_ENTITY_TO_FUNC(env_projectile);
LINK_ENTITY_TO_FUNC(env_rain);
LINK_ENTITY_TO_FUNC(env_render);
LINK_ENTITY_TO_FUNC(env_rotparticles);
LINK_ENTITY_TO_FUNC(env_shake);
LINK_ENTITY_TO_FUNC(env_shockwave);
LINK_ENTITY_TO_FUNC(env_shooter);
LINK_ENTITY_TO_FUNC(env_sky);
LINK_ENTITY_TO_FUNC(env_smoke);
LINK_ENTITY_TO_FUNC(env_smoker);
LINK_ENTITY_TO_FUNC(env_snow);
LINK_ENTITY_TO_FUNC(env_sound);
LINK_ENTITY_TO_FUNC(env_spark);
LINK_ENTITY_TO_FUNC(env_sprite);
LINK_ENTITY_TO_FUNC(env_state);
LINK_ENTITY_TO_FUNC(env_static);
LINK_ENTITY_TO_FUNC(env_sun);
LINK_ENTITY_TO_FUNC(env_teleporter);
LINK_ENTITY_TO_FUNC(env_warpball);
LINK_ENTITY_TO_FUNC(fireanddie);
LINK_ENTITY_TO_FUNC(flame_cloud);
LINK_ENTITY_TO_FUNC(flare);
LINK_ENTITY_TO_FUNC(func_ammocharge);
LINK_ENTITY_TO_FUNC(func_breakable);
LINK_ENTITY_TO_FUNC(func_breakable_model);
LINK_ENTITY_TO_FUNC(func_button);
LINK_ENTITY_TO_FUNC(func_conveyor);
LINK_ENTITY_TO_FUNC(func_door);
LINK_ENTITY_TO_FUNC(func_door_rotating);
LINK_ENTITY_TO_FUNC(func_friction);
LINK_ENTITY_TO_FUNC(func_guntarget);
LINK_ENTITY_TO_FUNC(func_healthcharger);
LINK_ENTITY_TO_FUNC(func_illusionary);
LINK_ENTITY_TO_FUNC(func_ladder);
LINK_ENTITY_TO_FUNC(func_lava);
LINK_ENTITY_TO_FUNC(func_monsterclip);
LINK_ENTITY_TO_FUNC(func_mortar_field);
LINK_ENTITY_TO_FUNC(func_pendulum);
LINK_ENTITY_TO_FUNC(func_plat);
LINK_ENTITY_TO_FUNC(func_platrot);
LINK_ENTITY_TO_FUNC(func_pushable);
LINK_ENTITY_TO_FUNC(func_recharge);
LINK_ENTITY_TO_FUNC(func_rot_button);
LINK_ENTITY_TO_FUNC(func_rotating);
LINK_ENTITY_TO_FUNC(func_tank);
LINK_ENTITY_TO_FUNC(func_tankcontrols);
LINK_ENTITY_TO_FUNC(func_tanklaser);
LINK_ENTITY_TO_FUNC(func_tankmortar);
LINK_ENTITY_TO_FUNC(func_tankrocket);
LINK_ENTITY_TO_FUNC(func_trackautochange);
LINK_ENTITY_TO_FUNC(func_trackchange);
LINK_ENTITY_TO_FUNC(func_tracktrain);
LINK_ENTITY_TO_FUNC(func_train);
LINK_ENTITY_TO_FUNC(func_traincontrols);
LINK_ENTITY_TO_FUNC(func_wall);
LINK_ENTITY_TO_FUNC(func_wall_computer);
LINK_ENTITY_TO_FUNC(func_wall_toggle);
LINK_ENTITY_TO_FUNC(func_water);
LINK_ENTITY_TO_FUNC(game_counter);
LINK_ENTITY_TO_FUNC(game_counter_set);
LINK_ENTITY_TO_FUNC(game_end);
LINK_ENTITY_TO_FUNC(game_player_equip);
LINK_ENTITY_TO_FUNC(game_player_hurt);
LINK_ENTITY_TO_FUNC(game_player_team);
LINK_ENTITY_TO_FUNC(game_score);
LINK_ENTITY_TO_FUNC(game_team_master);
LINK_ENTITY_TO_FUNC(game_team_set);
LINK_ENTITY_TO_FUNC(game_text);
LINK_ENTITY_TO_FUNC(game_zone_player);
LINK_ENTITY_TO_FUNC(garg_stomp);
LINK_ENTITY_TO_FUNC(gib);
LINK_ENTITY_TO_FUNC(gibshooter);
LINK_ENTITY_TO_FUNC(grenade);
LINK_ENTITY_TO_FUNC(hornet);
LINK_ENTITY_TO_FUNC(hud_icon);// XDM3037a
LINK_ENTITY_TO_FUNC(hud_timer);// XDM3038c
LINK_ENTITY_TO_FUNC(hvr_rocket);
LINK_ENTITY_TO_FUNC(info_alias);// SHL compatibility
LINK_ENTITY_TO_FUNC(info_bigmomma);
LINK_ENTITY_TO_FUNC(info_capture_obj);
LINK_ENTITY_TO_FUNC(info_ctfdetect);// Opposing Force CTF compatibility
LINK_ENTITY_TO_FUNC(info_ctfspawn);// Opposing Force CTF compatibility
LINK_ENTITY_TO_FUNC(info_dom_target);
LINK_ENTITY_TO_FUNC(info_group);// SHL compatibility
LINK_ENTITY_TO_FUNC(info_intermission);
LINK_ENTITY_TO_FUNC(info_landmark);
LINK_ENTITY_TO_FUNC(info_node);
LINK_ENTITY_TO_FUNC(info_node_air);
LINK_ENTITY_TO_FUNC(info_player_deathmatch);
LINK_ENTITY_TO_FUNC(info_player_start);
LINK_ENTITY_TO_FUNC(info_player_team1);// AGCTF compatibility
LINK_ENTITY_TO_FUNC(info_player_team2);// AGCTF compatibility
LINK_ENTITY_TO_FUNC(info_target);
LINK_ENTITY_TO_FUNC(info_teleport_destination);
LINK_ENTITY_TO_FUNC(infodecal);
LINK_ENTITY_TO_FUNC(isotopebox);
LINK_ENTITY_TO_FUNC(item);// XDM3038c
LINK_ENTITY_TO_FUNC(item_airtank);
LINK_ENTITY_TO_FUNC(item_antidote);
LINK_ENTITY_TO_FUNC(item_battery);
LINK_ENTITY_TO_FUNC(item_cd);
LINK_ENTITY_TO_FUNC(item_ctfflag);// ?
LINK_ENTITY_TO_FUNC(item_flag_team1);// AGCTF compatibility
LINK_ENTITY_TO_FUNC(item_flag_team2);// AGCTF compatibility
LINK_ENTITY_TO_FUNC(item_flare);
LINK_ENTITY_TO_FUNC(item_healthkit);
LINK_ENTITY_TO_FUNC(item_lambda);
LINK_ENTITY_TO_FUNC(item_longjump);
LINK_ENTITY_TO_FUNC(item_security);
LINK_ENTITY_TO_FUNC(item_sodacan);
LINK_ENTITY_TO_FUNC(item_suit);
LINK_ENTITY_TO_FUNC(l_grenade);
LINK_ENTITY_TO_FUNC(laser_spot);
LINK_ENTITY_TO_FUNC(lava_ball);
LINK_ENTITY_TO_FUNC(light);
LINK_ENTITY_TO_FUNC(light_environment);
LINK_ENTITY_TO_FUNC(light_spot);
LINK_ENTITY_TO_FUNC(lightp);
LINK_ENTITY_TO_FUNC(locus_alias);// SHL compatibility
LINK_ENTITY_TO_FUNC(locus_beam);// SHL compatibility
LINK_ENTITY_TO_FUNC(locus_variable);// SHL compatibility
LINK_ENTITY_TO_FUNC(momentary_door);
LINK_ENTITY_TO_FUNC(momentary_rot_button);
LINK_ENTITY_TO_FUNC(monster_alien_controller);
LINK_ENTITY_TO_FUNC(monster_alien_grunt);
LINK_ENTITY_TO_FUNC(monster_alien_slave);
LINK_ENTITY_TO_FUNC(monster_alien_voltigore);// OpForce compatibility
LINK_ENTITY_TO_FUNC(monster_apache);
LINK_ENTITY_TO_FUNC(monster_babycrab);
LINK_ENTITY_TO_FUNC(monster_barnacle);
LINK_ENTITY_TO_FUNC(monster_barney);
LINK_ENTITY_TO_FUNC(monster_barney_dead);
LINK_ENTITY_TO_FUNC(monster_bigmomma);
LINK_ENTITY_TO_FUNC(monster_bloater);
LINK_ENTITY_TO_FUNC(monster_bullchicken);
LINK_ENTITY_TO_FUNC(monster_cat);
LINK_ENTITY_TO_FUNC(monster_cockroach);
LINK_ENTITY_TO_FUNC(monster_flyer);
LINK_ENTITY_TO_FUNC(monster_flyer_flock);
LINK_ENTITY_TO_FUNC(monster_friendly);
LINK_ENTITY_TO_FUNC(monster_furniture);
LINK_ENTITY_TO_FUNC(monster_gargantua);
LINK_ENTITY_TO_FUNC(monster_generic);
LINK_ENTITY_TO_FUNC(monster_gman);
LINK_ENTITY_TO_FUNC(monster_gonome);
LINK_ENTITY_TO_FUNC(monster_grunt_repel);
LINK_ENTITY_TO_FUNC(monster_headcrab);
LINK_ENTITY_TO_FUNC(monster_hevsuit_dead);
LINK_ENTITY_TO_FUNC(monster_hgrunt_dead);
LINK_ENTITY_TO_FUNC(monster_houndeye);
LINK_ENTITY_TO_FUNC(monster_human_assassin);
LINK_ENTITY_TO_FUNC(monster_human_grunt);
LINK_ENTITY_TO_FUNC(monster_ichthyosaur);
LINK_ENTITY_TO_FUNC(monster_kingpin);
LINK_ENTITY_TO_FUNC(monster_leech);
LINK_ENTITY_TO_FUNC(monster_male_assassin);// OpForce compatibility
LINK_ENTITY_TO_FUNC(monster_miniturret);
LINK_ENTITY_TO_FUNC(monster_mortar);
LINK_ENTITY_TO_FUNC(monster_nihilanth);
LINK_ENTITY_TO_FUNC(monster_osprey);
LINK_ENTITY_TO_FUNC(monster_panther);
LINK_ENTITY_TO_FUNC(monster_pitdrone);// OpForce compatibility
LINK_ENTITY_TO_FUNC(monster_pitworm);// OpForce compatibility
LINK_ENTITY_TO_FUNC(monster_player);
LINK_ENTITY_TO_FUNC(monster_rat);
LINK_ENTITY_TO_FUNC(monster_robo_grunt);
LINK_ENTITY_TO_FUNC(monster_satchel);
LINK_ENTITY_TO_FUNC(monster_scientist);
LINK_ENTITY_TO_FUNC(monster_scientist_dead);
LINK_ENTITY_TO_FUNC(monster_sentry);
LINK_ENTITY_TO_FUNC(monster_shocktrooper);// OpForce compatibility
LINK_ENTITY_TO_FUNC(monster_sitting_scientist);
LINK_ENTITY_TO_FUNC(monster_snark);
//LINK_ENTITY_TO_FUNC(monster_space_ship);
LINK_ENTITY_TO_FUNC(monster_target);
LINK_ENTITY_TO_FUNC(monster_tentacle);
//LINK_ENTITY_TO_FUNC(monster_tentaclemaw);
LINK_ENTITY_TO_FUNC(monster_tripmine);
LINK_ENTITY_TO_FUNC(monster_turret);
LINK_ENTITY_TO_FUNC(monster_vortigaunt);
LINK_ENTITY_TO_FUNC(monster_zombie);
LINK_ENTITY_TO_FUNC(monstermaker);
LINK_ENTITY_TO_FUNC(multi_alias);// SHL compatibility
LINK_ENTITY_TO_FUNC(multi_manager);
LINK_ENTITY_TO_FUNC(multi_switcher);// SHL compatibility
LINK_ENTITY_TO_FUNC(multi_watcher);// SHL compatibility
LINK_ENTITY_TO_FUNC(multisource);
LINK_ENTITY_TO_FUNC(nihilanth_energy_ball);
LINK_ENTITY_TO_FUNC(node_viewer);
LINK_ENTITY_TO_FUNC(node_viewer_fly);
LINK_ENTITY_TO_FUNC(node_viewer_human);
LINK_ENTITY_TO_FUNC(node_viewer_large);
LINK_ENTITY_TO_FUNC(nucdevice);
LINK_ENTITY_TO_FUNC(orb);
LINK_ENTITY_TO_FUNC(path_corner);
LINK_ENTITY_TO_FUNC(path_track);
LINK_ENTITY_TO_FUNC(plasmaball);
LINK_ENTITY_TO_FUNC(player);
LINK_ENTITY_TO_FUNC(player_loadsaved);
LINK_ENTITY_TO_FUNC(player_weaponstrip);
LINK_ENTITY_TO_FUNC(razordisk);
LINK_ENTITY_TO_FUNC(rpg_rocket);
LINK_ENTITY_TO_FUNC(scripted_sentence);
LINK_ENTITY_TO_FUNC(scripted_sequence);
LINK_ENTITY_TO_FUNC(soundent);
LINK_ENTITY_TO_FUNC(speaker);
LINK_ENTITY_TO_FUNC(squeakbox);
LINK_ENTITY_TO_FUNC(squidspit);
LINK_ENTITY_TO_FUNC(streak_spiral);
LINK_ENTITY_TO_FUNC(strtarget);
LINK_ENTITY_TO_FUNC(target_cdaudio);
LINK_ENTITY_TO_FUNC(tele_ball);
LINK_ENTITY_TO_FUNC(teleporter);
//LINK_ENTITY_TO_FUNC(teleporter_spiral);
LINK_ENTITY_TO_FUNC(test_effect);
LINK_ENTITY_TO_FUNC(testhull);
LINK_ENTITY_TO_FUNC(trigger);
LINK_ENTITY_TO_FUNC(trigger_auto);
LINK_ENTITY_TO_FUNC(trigger_autosave);
LINK_ENTITY_TO_FUNC(trigger_bounce);
LINK_ENTITY_TO_FUNC(trigger_camera);
LINK_ENTITY_TO_FUNC(trigger_cap_point);
LINK_ENTITY_TO_FUNC(trigger_cdaudio);
LINK_ENTITY_TO_FUNC(trigger_changealias);// SHL compatibility
LINK_ENTITY_TO_FUNC(trigger_changelevel);
LINK_ENTITY_TO_FUNC(trigger_changetarget);
LINK_ENTITY_TO_FUNC(trigger_counter);
LINK_ENTITY_TO_FUNC(trigger_dom_point);
LINK_ENTITY_TO_FUNC(trigger_endsection);
LINK_ENTITY_TO_FUNC(trigger_gravity);
LINK_ENTITY_TO_FUNC(trigger_hurt);
LINK_ENTITY_TO_FUNC(trigger_monsterjump);
LINK_ENTITY_TO_FUNC(trigger_multiple);
LINK_ENTITY_TO_FUNC(trigger_once);
LINK_ENTITY_TO_FUNC(trigger_onsight);
LINK_ENTITY_TO_FUNC(trigger_platcontrol);// XDM3038c
LINK_ENTITY_TO_FUNC(trigger_playerfreeze);// XDM3035c
LINK_ENTITY_TO_FUNC(trigger_push);
LINK_ENTITY_TO_FUNC(trigger_relay);
LINK_ENTITY_TO_FUNC(trigger_secret);// XDM3038c
LINK_ENTITY_TO_FUNC(trigger_sound);
LINK_ENTITY_TO_FUNC(trigger_startpatrol);
LINK_ENTITY_TO_FUNC(trigger_teleport);
LINK_ENTITY_TO_FUNC(trigger_transition);
//LINK_ENTITY_TO_FUNC(trip_beam);
LINK_ENTITY_TO_FUNC(weapon);
LINK_ENTITY_TO_FUNC(weapon_357);
LINK_ENTITY_TO_FUNC(weapon_9mmAR);
LINK_ENTITY_TO_FUNC(weapon_9mmhandgun);
LINK_ENTITY_TO_FUNC(weapon_alauncher);
LINK_ENTITY_TO_FUNC(weapon_beamrifle);
LINK_ENTITY_TO_FUNC(weapon_bhg);
LINK_ENTITY_TO_FUNC(weapon_chemgun);
LINK_ENTITY_TO_FUNC(weapon_crossbow);
LINK_ENTITY_TO_FUNC(weapon_crowbar);
LINK_ENTITY_TO_FUNC(weapon_custom);
LINK_ENTITY_TO_FUNC(weapon_dlauncher);
LINK_ENTITY_TO_FUNC(weapon_displacer);
LINK_ENTITY_TO_FUNC(weapon_egon);
LINK_ENTITY_TO_FUNC(weapon_flame);
LINK_ENTITY_TO_FUNC(weapon_gauss);
LINK_ENTITY_TO_FUNC(weapon_glauncher);
LINK_ENTITY_TO_FUNC(weapon_glock);
LINK_ENTITY_TO_FUNC(weapon_handgrenade);
LINK_ENTITY_TO_FUNC(weapon_hornetgun);
LINK_ENTITY_TO_FUNC(weapon_mp5);
LINK_ENTITY_TO_FUNC(weapon_plasma);
LINK_ENTITY_TO_FUNC(weapon_python);
LINK_ENTITY_TO_FUNC(weapon_rpg);
LINK_ENTITY_TO_FUNC(weapon_satchel);
LINK_ENTITY_TO_FUNC(weapon_shotgun);
LINK_ENTITY_TO_FUNC(weapon_snark);
LINK_ENTITY_TO_FUNC(weapon_sniperrifle);
LINK_ENTITY_TO_FUNC(weapon_strtarget);
LINK_ENTITY_TO_FUNC(weapon_sword);
LINK_ENTITY_TO_FUNC(weapon_tripmine);
LINK_ENTITY_TO_FUNC(weapon_translocator);
LINK_ENTITY_TO_FUNC(weaponbox);
LINK_ENTITY_TO_FUNC(world_items);
LINK_ENTITY_TO_FUNC(worldspawn);
LINK_ENTITY_TO_FUNC(xen_hair);
LINK_ENTITY_TO_FUNC(xen_hull);
LINK_ENTITY_TO_FUNC(xen_plantlight);
LINK_ENTITY_TO_FUNC(xen_spore_large);
LINK_ENTITY_TO_FUNC(xen_spore_medium);
LINK_ENTITY_TO_FUNC(xen_spore_small);
LINK_ENTITY_TO_FUNC(xen_tree);
LINK_ENTITY_TO_FUNC(xen_ttrigger);
LINK_ENTITY_TO_FUNC(zone_register);// SHL compatibility

/*
monster_alien_babyvoltigore
monster_alien_voltigore
monster_alien_slave_dead
monster_assassin_repel
monster_blkop_apache
monster_blkop_osprey
monster_cleansuit_scientist
monster_cleansuit_scientist_dead
monster_sitting_cleansuit_scientist
monster_drillsergeant
monster_geneworm
monster_gonome
monster_gonome_dead
monster_houndeye_dead
monster_grunt_ally_repel
monster_human_grunt_ally
monster_human_grunt_ally_dead
monster_human_medic_ally
monster_human_torch_ally

monster_op4loader
monster_otis
monster_otis_dead


monster_pitworm_up
monster_shockroach
monster_zombie_barney
monster_zombie_soldier
monster_zombie_soldier_dead
monster_recruit
monster_skeleton_dead

// ghoul-made classnames:
LINK_ENTITY_TO_FUNC(kingpinbolt);
LINK_ENTITY_TO_FUNC(monster_assassin_repel);
LINK_ENTITY_TO_FUNC(monster_gargantua_stone);
LINK_ENTITY_TO_FUNC(monster_gargantua_baby);
LINK_ENTITY_TO_FUNC(monster_headcrab_fast);
LINK_ENTITY_TO_FUNC(monster_headcrab_poison);
LINK_ENTITY_TO_FUNC(monster_hwgrunt);
LINK_ENTITY_TO_FUNC(monster_pit_drone);
LINK_ENTITY_TO_FUNC(monster_pit_worm);
LINK_ENTITY_TO_FUNC(monster_male_assassin);
LINK_ENTITY_TO_FUNC(monster_shocktrooper);
LINK_ENTITY_TO_FUNC(monster_stinger);
LINK_ENTITY_TO_FUNC(monster_voltigore);
LINK_ENTITY_TO_FUNC(monster_voltigore_baby);
LINK_ENTITY_TO_FUNC(monster_zombie_robot);
LINK_ENTITY_TO_FUNC(monster_zombie_barney);
LINK_ENTITY_TO_FUNC(monster_zombie_soldier);
LINK_ENTITY_TO_FUNC(monster_zombie_blackop);
LINK_ENTITY_TO_FUNC(monster_zombie_construction);
LINK_ENTITY_TO_FUNC(monster_zombie_torso);
LINK_ENTITY_TO_FUNC(monster_zombie_fast);
LINK_ENTITY_TO_FUNC(monster_zombie_poison);
*/

// Spirit of Half-Life compatibility
//LINK_ENTITY_TO_FUNC(calc_angletransform);// SHL compatibility
//LINK_ENTITY_TO_FUNC(env_particle);// SHL compatibility
