#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ammohistory.h"

// keep a list of items
void HistoryResource::Init(void)
{
	Reset();
}

void HistoryResource::Reset(void)
{
	memset(rgAmmoHistory, 0, sizeof(rgAmmoHistory));
	iCurrentHistorySlot = 0;
}

void HistoryResource::AddToHistory(const int &iType, const int &iId, const int &iCount)
{
	if (iType == HISTSLOT_AMMO && iCount == 0)// ???
		return;

	if (iCurrentHistorySlot >= PICKUP_HISTORY_MAX)
		iCurrentHistorySlot = 0;

	HIST_ITEM *freeslot = &rgAmmoHistory[iCurrentHistorySlot];
	freeslot->type = iType;
	freeslot->iId = iId;
	freeslot->iCount = iCount;
	freeslot->DisplayTime = gHUD.m_flTime + gHUD.m_Ammo.m_pCvarDrawHistoryTime->value;
	++iCurrentHistorySlot; 
}

void HistoryResource::CheckClearHistory(void)
{
	for (size_t i = 0; i < PICKUP_HISTORY_MAX; ++i)
	{
		if (rgAmmoHistory[i].type != HISTSLOT_EMPTY)
			return;
	}
	iCurrentHistorySlot = 0;
}

int HistoryResource::DrawAmmoHistory(const float &flTime)
{
	int ypos = gHUD.m_Ammo.m_iHeight - PICKUP_HISTORY_MARGIN;//GetAmmoDisplayPosY() - PICKUP_HISTORY_MARGIN;
	if (ypos == 0)// failed?
		ypos = ScreenHeight - PICKUP_HISTORY_MARGIN;

	for (size_t i = 0; i < PICKUP_HISTORY_MAX; ++i)
	{
		if (rgAmmoHistory[i].type)
		{
			rgAmmoHistory[i].DisplayTime = min(rgAmmoHistory[i].DisplayTime, gHUD.m_flTime + gHUD.m_Ammo.m_pCvarDrawHistoryTime->value);
			if (rgAmmoHistory[i].DisplayTime <= flTime)// pic drawing time has expired
			{
				memset(&rgAmmoHistory[i], 0, sizeof(HIST_ITEM));
				CheckClearHistory();
			}
			else if (rgAmmoHistory[i].type == HISTSLOT_AMMO)
			{
				wrect_t rcPic;
#if defined (OLD_WEAPON_AMMO_INFO)
				HLSPRITE *spr = gHUD.m_Ammo.gWR.GetAmmoPicFromWeapon( rgAmmoHistory[i].iId, rcPic );
#else
				HLSPRITE hTemp = gHUD.m_Ammo.AmmoSpriteHandle(rgAmmoHistory[i].iId);
				HLSPRITE *spr = &hTemp;
				rcPic = gHUD.m_Ammo.AmmoSpriteRect(rgAmmoHistory[i].iId);
#endif
				int r, g, b;
				UnpackRGB(r,g,b, RGB_GREEN);
				float scale = (rgAmmoHistory[i].DisplayTime - flTime) * 80;
				ScaleColors(r, g, b, min((int)scale, 255));
				// Draw the pic
				// what the shit is this?!	int ypos = ScreenHeight - (AMMO_PICKUP_PICK_HEIGHT + (AMMO_PICKUP_GAP * i));
				int xpos = ScreenWidth - RectWidth(rcPic);
				ypos -= (RectHeight(rcPic) + PICKUP_HISTORY_MARGIN);
				if (spr && *spr)// weapon isn't loaded yet so just don't draw the pic
				{
					SPR_Set(*spr, r, g, b);
					SPR_DrawAdditive(0, xpos, ypos, &rcPic);
				}
				if (rgAmmoHistory[i].iCount > 0)// XDM3038c: ?
					gHUD.DrawHudNumberString(xpos - 10, ypos, xpos - 100, rgAmmoHistory[i].iCount, r, g, b);
			}
			else if (rgAmmoHistory[i].type == HISTSLOT_WEAP)
			{
				HUD_WEAPON *weap = gHUD.m_Ammo.gWR.GetWeaponStruct(rgAmmoHistory[i].iId);
				if (weap == NULL)
					return 1;  // we don't know about the weapon yet, so don't draw anything

				int r, g, b;
				if (gHUD.m_Ammo.gWR.IsSelectable(weap))// XDM3037: fixed
					UnpackRGB(r,g,b, RGB_GREEN);
				else
					UnpackRGB(r,g,b, RGB_YELLOW);// if the weapon doesn't have ammo, display it as yellow

				float scale = (rgAmmoHistory[i].DisplayTime - flTime) * 80;
				ScaleColors(r, g, b, min((int)scale, 255));

				// what the shit is this?!	int ypos = ScreenHeight - (AMMO_PICKUP_PICK_HEIGHT + (AMMO_PICKUP_GAP * i));
				int xpos = ScreenWidth - RectWidth(weap->rcInactive);// XDM3037a: (weap->rcInactive.right - weap->rcInactive.left);
				ypos -= RectHeight(weap->rcInactive) + PICKUP_HISTORY_MARGIN;//(weap->rcInactive.bottom - weap->rcInactive.top + PICKUP_HISTORY_MARGIN);

				SPR_Set(weap->hInactive, r, g, b);
				SPR_DrawAdditive(0, xpos, ypos, &weap->rcInactive);
			}
			else if (rgAmmoHistory[i].type == HISTSLOT_ITEM)
			{
				if (rgAmmoHistory[i].iId == 0)
					continue;// sprite not loaded

				wrect_t rect = gHUD.GetSpriteRect(rgAmmoHistory[i].iId);
				int r, g, b;
				UnpackRGB(r,g,b, RGB_GREEN);
				float scale = (rgAmmoHistory[i].DisplayTime - flTime) * 80;
				ScaleColors(r, g, b, min((int)scale, 255));
				// what the shit is this?!	int ypos = ScreenHeight - (AMMO_PICKUP_PICK_HEIGHT + (AMMO_PICKUP_GAP * i));
				int xpos = ScreenWidth - RectWidth(rect)-10;// XDM3037a: (rect.right - rect.left) - 10;
				ypos -= RectHeight(rect) + PICKUP_HISTORY_MARGIN;//(rect.bottom - rect.top + PICKUP_HISTORY_MARGIN);
				SPR_Set(gHUD.GetSprite(rgAmmoHistory[i].iId), r, g, b);
				SPR_DrawAdditive(0, xpos, ypos, &rect);
				// Draw the number
				if (rgAmmoHistory[i].iCount > 0)// XDM3035a
					gHUD.DrawHudNumberString(xpos - 10, ypos, xpos - 100, rgAmmoHistory[i].iCount, r, g, b);
			}
		}
		if (ypos < (ScreenHeight/4))// 1/4th of the screen
		{
			break;// just don't draw any following items (they'll probably get drawn when the list will start collapsing)
			//ypos = ScreenHeight;// reset
			//m_bOverflow = true;
		}
	}
	return 1;
}
