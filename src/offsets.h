#pragma once

#include <windows.h>

struct offsets_t
{
public:
	DWORD dwViewPunchAngles;
	DWORD dwEyeAngles;
	DWORD dwWriteUserCmd;
	DWORD dwEyePosOffset;
	DWORD dwhActiveWeapon;
	DWORD dwVecPunchAngles;
	DWORD dwLocal;
	DWORD dwflFlashMaxAlpha;
	DWORD dwflFlashDuration;
	DWORD dwbIsScoped;
	DWORD dwSpottedMask;
	DWORD dwfFlags;
	DWORD dwFogEnable;
	DWORD dwWeaponOwnerHandle;
	DWORD dwTickBase; // = 0x17a4;
	DWORD dwNextAttack;
	DWORD dwNextPrimaryAttack;
	DWORD dwTraceLine;
	DWORD dwUtilSmoke;
	DWORD dwWeaponIdToString;
	DWORD dwLookupWeaponInfoSlot;
	DWORD dwGetFileWeaponInfoFromHandle;
	DWORD dwBloodDrips;
	DWORD dwPR;
	DWORD dwPlayerState;
	DWORD dwClientBase;
	DWORD dwEngineBase;
	DWORD dwFileSysBase;
	DWORD dwMatSysBase;
};

extern offsets_t offys;