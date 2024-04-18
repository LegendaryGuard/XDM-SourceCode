//=========================================================
// skill.h - skill level concerns
//=========================================================
#ifndef SKILL_H
#define SKILL_H

enum skill_level_e
{
	SKILL_NONE = 0,// ???
	SKILL_EASY,
	SKILL_MEDIUM,
	SKILL_HARD
};

struct skilldata_t
{
	int iSkillLevel;// currently loaded skill level

// Monster Health & Damage
	float agruntDmgPunch;
	float agruntHealth;
	float agruntScore;

	float apacheHealth;
	float apacheScore;

	float barnacleDmgBite;
	float barnacleHealth;
	float barnacleScore;

	float barneyHealth;
	float barneyScore;

	float bigmommaDmgSlash;
	float bigmommaDmgBlast;
	float bigmommaHealthFactor;// Multiply each node's health by this
	//float bigmommaRadiusBlast;
	float bigmommaMaxChildren;// XDM
	float bigmommaScore;

	float bullsquidDmgBite;
	float bullsquidDmgSpit;
	float bullsquidDmgWhip;
	float bullsquidHealth;
	float bullsquidScore;

	float controllerDmgBall;
	float controllerDmgZap;
	float controllerHealth;
	float controllerScore;

	float gargantuaDmgFire;
	float gargantuaDmgFireBall;
	float gargantuaDmgSlash;
	float gargantuaDmgStomp;
	float gargantuaHealth;
	float gargantuaScore;

	float gonomeDmgBite;
	float gonomeDmgJump;
	float gonomeDmgSpit;
	float gonomeHealth;
	float gonomeScore;

	float hassassinHealth;
	float hassassinScore;
	float massassinHealth;
	float massassinScore;

	float headcrabDmgBite;
	float headcrabHealth;
	float headcrabScore;

	float hgruntDmgKick;
	float hgruntGrenadeSpeed;
	float hgruntHealth;
	float hgruntScore;

	float houndeyeHealth;
	float houndeyeDmgBlast;
	float houndeyeScore;
/*
	float hwgruntDmgBullet;
	float hwgruntDmgGrenade;
	float hwgruntHealth;
	float hwgruntScore;
*/
	float ichthyosaurHealth;
	float ichthyosaurDmgShake;
	float ichthyosaurScore;
/*
	float kingpinDmgBite;
	float kingpinDmgBolt;
	float kingpinHealth;
	float kingpinScore;
*/
	float leechDmgBite;
	float leechHealth;
	float leechScore;

	float ospreyHealth;
	float ospreyScore;

	float pantherDmgSlash;
	float pantherDmgJump;
	float pantherHealth;
	float pantherScore;

	float pitdroneDmgBite;
	float pitdroneDmgSpit;
	float pitdroneHealth;
	float pitdroneScore;

/*	float pitwormDmgBeam;
	float pitwormDmgSlash;
	float pitwormDmgScream;
	float pitwormDmgSpit;
	float pitwormDmgTeleport;
	float pitwormHealth;
	float pitwormScore;*/

	float nihilanthHealth;
	float nihilanthZap;
	float nihilanthScore;

	float ratDmgBite;
	float ratHealth;
	float ratScore;

	float rgruntHealth;
	float rgruntScore;

	float scientistHealth;
	float scientistScore;

	float shocktrooperDmgGrenade;
	float shocktrooperDmgKick;
	float shocktrooperDmgShock;
	//float shocktrooperDmgSpore;
	//float shocktrooperDmgHornet;
	float shocktrooperHealth;
	float shocktrooperScore;

	float slaveDmgClaw;
	// UNUSED float slaveDmgClawrake;
	float slaveDmgZap;
	float slaveHealth;
	float slaveScore;

	float snarkHealth;
	float snarkDmgBite;
	float snarkDmgPop;

	float tentacleDmg;
	float tentacleHealth;
	float tentacleHideTime;
	float tentacleScore;

	float turretHealth;
	float turretScore;
	float miniturretHealth;
	float miniturretScore;
	float sentryHealth;
	float sentryScore;

	float voltigoreDmgBite;
	float voltigoreDmgBolt;
	float voltigoreHealth;
	float voltigoreScore;

	float zombieDmgSlash;
	float zombieHealth;
	float zombieScore;
/*
	float zrobotDmgJump;
	float zrobotDmgFire;
	float zrobotDmgLaser;
	float zrobotDmgSlash;
	float zrobotHealth;
	float zrobotScore;
*/
// Player Weapons
	float DmgCrowbar;
	float Dmg9MM;
	float Dmg357;
	float DmgMP5;
	float DmgM203Grenade;
	float DmgBolt;// XDM3038c
	float DmgBoltExplode;// XDM3038c
	float DmgBuckshot;
	float DmgRPG;
	float DmgGauss;
	float DmgEgon;
	float DmgHornet;
	float DmgGrenadeLauncher;
	float DmgHandGrenadeExplosive;// XDM3037a
	float DmgHandGrenadeFreeze;
	float DmgHandGrenadePoison;
	float DmgHandGrenadeBurn;
	float DmgHandGrenadeRadiation;
	float DmgSatchel;
	float DmgStrikeTarget;
	float DmgTripmine;
	float DmgAcidGrenade;
	float DmgDisplacerBeam;
	float DmgDisplacerBlast;
	float DmgChemgun;
	float DmgPlasma;
	float DmgFlame;
	float DmgNuclear;
	float DmgSword;
	float DmgBeamRifle;
	float DmgPlasmaBall;
	float DmgRazorDisk;
	float DmgSniperRifle;
	float DmgBHG;
	float FlashlightCharge;
	float FlashlightDrain;
	float PlrAirTime;
// weapons shared by monsters
	float monDmg9MM;
	float monDmgMP5;
	float Dmg12MM;
	float monDmgHornet;
// world objects
	float DmgMortar;

// health/suit charge
	float suitchargerCapacity;
	float batteryCapacity;
	float healthchargerCapacity;
	float healthkitCapacity;
	float scientistHeal;
	float foodHeal;
// monster damage adj
	float monHead;
	float monChest;
	float monStomach;
	float monLeg;
	float monArm;
// player damage adj
	float plrHead;
	float plrChest;
	float plrStomach;
	float plrLeg;
	float plrArm;
	float plrScore;
};

extern DLL_GLOBAL skilldata_t gSkillData;
//extern DLL_GLOBAL int g_iSkillLevel;

void SkillRegisterCvars(void);
float GetSkillCvar(const cvar_t *pCVar, const float &fValueMin, const float &fValueMax);
void SkillUpdateData(int iSkill);

#endif // SKILL_H
