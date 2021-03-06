//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "reslib.h"
#include "ships/druuge/resinst.h"
#include "starcon.h"

#define MAX_CREW 14
#define MAX_ENERGY 32
#define ENERGY_REGENERATION 1
#define WEAPON_ENERGY_COST 4
#define SPECIAL_ENERGY_COST 16
#define ENERGY_WAIT 50
#define MAX_THRUST 20
#define THRUST_INCREMENT 2
#define TURN_WAIT 4
#define THRUST_WAIT 1
#define WEAPON_WAIT 10
#define SPECIAL_WAIT 30

#define SHIP_MASS 5
#define MISSILE_SPEED DISPLAY_TO_WORLD (30)
#define MISSILE_LIFE 20
#define MISSILE_RANGE (MISSILE_SPEED * MISSILE_LIFE)

static RACE_DESC druuge_desc =
{
	{
		FIRES_FORE,
		17, /* Super Melee cost */
		1400 / SPHERE_RADIUS_INCREMENT, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		{
			9500, 2792,
		},
		(STRING)DRUUGE_RACE_STRINGS,
		(FRAME)DRUUGE_ICON_MASK_PMAP_ANIM,
		(FRAME)DRUUGE_MICON_MASK_PMAP_ANIM,
	},
	{
		MAX_THRUST,
		THRUST_INCREMENT,
		ENERGY_REGENERATION,
		WEAPON_ENERGY_COST,
		SPECIAL_ENERGY_COST,
		ENERGY_WAIT,
		TURN_WAIT,
		THRUST_WAIT,
		WEAPON_WAIT,
		SPECIAL_WAIT,
		SHIP_MASS,
	},
	{
		{
			(FRAME)DRUUGE_BIG_MASK_PMAP_ANIM,
			(FRAME)DRUUGE_MED_MASK_PMAP_ANIM,
			(FRAME)DRUUGE_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)CANNON_BIG_MASK_PMAP_ANIM,
			(FRAME)CANNON_MED_MASK_PMAP_ANIM,
			(FRAME)CANNON_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		{
			(FRAME)DRUUGE_CAPT_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)DRUUGE_VICTORY_SONG,
		(SOUND)DRUUGE_SHIP_SOUNDS,
	},
	{
		0,
		MISSILE_RANGE,
		NULL_PTR,
	},
	NULL_PTR,
	NULL_PTR,
	NULL_PTR,
	0,
};

#define RECOIL_VELOCITY WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (6))
#define MAX_RECOIL_VELOCITY (RECOIL_VELOCITY * 4)

static void
cannon_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);

	if ((ElementPtr1->state_flags & PLAYER_SHIP)
			&& ElementPtr1->crew_level
			&& !GRAVITY_MASS (ElementPtr1->mass_points + 1))
	{
		COUNT angle;
		SIZE cur_delta_x, cur_delta_y;
		STARSHIPPTR StarShipPtr;

		GetElementStarShip (ElementPtr1, &StarShipPtr);
		StarShipPtr->cur_status_flags &=
				~(SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);

		angle = FACING_TO_ANGLE (
				GetFrameIndex (ElementPtr0->next.image.frame)
				);
		DeltaVelocityComponents (&ElementPtr1->velocity,
				COSINE (angle, RECOIL_VELOCITY),
				SINE (angle, RECOIL_VELOCITY));
		GetCurrentVelocityComponents (&ElementPtr1->velocity,
				&cur_delta_x, &cur_delta_y);
		if ((long)cur_delta_x * (long)cur_delta_x
				+ (long)cur_delta_y * (long)cur_delta_y
				> (long)MAX_RECOIL_VELOCITY * (long)MAX_RECOIL_VELOCITY)
		{
			angle = ARCTAN (cur_delta_x, cur_delta_y);
			SetVelocityComponents (&ElementPtr1->velocity,
					COSINE (angle, MAX_RECOIL_VELOCITY),
					SINE (angle, MAX_RECOIL_VELOCITY));
		}
	}
}

static COUNT
initialize_cannon (PELEMENT ShipPtr, HELEMENT CannonArray[])
{
#define DRUUGE_OFFSET 24
#define MISSILE_OFFSET 6
#define MISSILE_HITS 4
#define MISSILE_DAMAGE 6
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = MissileBlock.face;
	MissileBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	MissileBlock.pixoffs = DRUUGE_OFFSET;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL_PTR;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	CannonArray[0] = initialize_missile (&MissileBlock);

	if (CannonArray[0])
	{
		ELEMENTPTR CannonPtr;

		LockElement (CannonArray[0], &CannonPtr);
		CannonPtr->collision_func = cannon_collision;
		UnlockElement (CannonArray[0]);
	}

	return (1);
}

static void
druuge_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter)
PELEMENT ShipPtr;
PEVALUATE_DESC ObjectsOfConcern;
COUNT ConcernCounter;
{
	UWORD ship_flags;
	STARSHIPPTR StarShipPtr, EnemyStarShipPtr;
	PEVALUATE_DESC lpEvalDesc;

	GetElementStarShip (ShipPtr, &StarShipPtr);

	ship_flags = 0;
	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (StarShipPtr->cur_status_flags & SHIP_BEYOND_MAX_SPEED)
		lpEvalDesc->MoveState = ENTICE;
	else if (lpEvalDesc->ObjectPtr
			&& lpEvalDesc->which_turn <= WORLD_TO_TURN (MISSILE_RANGE * 3 / 4))
	{
		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		ship_flags = EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags;
		EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags &=
				~(FIRES_FORE | FIRES_RIGHT | FIRES_AFT | FIRES_LEFT);

		lpEvalDesc->MoveState = PURSUE;
		if (ShipPtr->thrust_wait == 0)
			++ShipPtr->thrust_wait;
	}
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
	if (ship_flags)
	{
		EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags = ship_flags;
	}

	if (!(StarShipPtr->cur_status_flags & SHIP_BEYOND_MAX_SPEED)
			&& (lpEvalDesc->which_turn <= 12
			|| (
			ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr
			&& ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn <= 6
			)))
	{
		 StarShipPtr->ship_input_state |= WEAPON;
		 if (ShipPtr->thrust_wait < WEAPON_WAIT + 1)
			ShipPtr->thrust_wait = WEAPON_WAIT + 1;
	}


	if ((StarShipPtr->ship_input_state & WEAPON)
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level < WEAPON_ENERGY_COST
			&& ShipPtr->crew_level > 1)
		StarShipPtr->ship_input_state |= SPECIAL;
	else
		StarShipPtr->ship_input_state &= ~SPECIAL;
}

static void
druuge_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
			/* if just fired cannon */
	if ((StarShipPtr->cur_status_flags & WEAPON)
			&& StarShipPtr->weapon_counter ==
			StarShipPtr->RaceDescPtr->characteristics.weapon_wait)
	{
		COUNT angle;
		SIZE cur_delta_x, cur_delta_y;

		StarShipPtr->cur_status_flags &= ~SHIP_AT_MAX_SPEED;

		angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing) + HALF_CIRCLE;
		DeltaVelocityComponents (&ElementPtr->velocity,
				COSINE (angle, RECOIL_VELOCITY),
				SINE (angle, RECOIL_VELOCITY));
		GetCurrentVelocityComponents (&ElementPtr->velocity,
				&cur_delta_x, &cur_delta_y);
		if ((long)cur_delta_x * (long)cur_delta_x
				+ (long)cur_delta_y * (long)cur_delta_y
				> (long)MAX_RECOIL_VELOCITY * (long)MAX_RECOIL_VELOCITY)
		{
			angle = ARCTAN (cur_delta_x, cur_delta_y);
			SetVelocityComponents (&ElementPtr->velocity,
					COSINE (angle, MAX_RECOIL_VELOCITY),
					SINE (angle, MAX_RECOIL_VELOCITY));
		}
	}
}

static void
druuge_preprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->cur_status_flags & SPECIAL)
	{
		if (StarShipPtr->special_counter
				|| ElementPtr->crew_level == 1
				|| StarShipPtr->RaceDescPtr->ship_info.energy_level
				== StarShipPtr->RaceDescPtr->ship_info.max_energy)
			StarShipPtr->cur_status_flags &= ~SPECIAL;
		else
		{
			ProcessSound (SetAbsSoundIndex (
							/* BURN UP CREW */
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1));

			DeltaCrew (ElementPtr, -1);
			DeltaEnergy (ElementPtr, SPECIAL_ENERGY_COST);

			StarShipPtr->special_counter =
					StarShipPtr->RaceDescPtr->characteristics.special_wait;
		}
	}
}

RACE_DESCPTR
init_druuge ()
{
	RACE_DESCPTR RaceDescPtr;

	druuge_desc.preprocess_func = druuge_preprocess;
	druuge_desc.postprocess_func = druuge_postprocess;
	druuge_desc.init_weapon_func = initialize_cannon;
	druuge_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) druuge_intelligence;

	RaceDescPtr = &druuge_desc;

	return (RaceDescPtr);
}

