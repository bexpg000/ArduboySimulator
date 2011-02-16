#include "PlatformPrecomp.h"
#include "TouchDragComponent.h"
#include "BaseApp.h"

TouchDragComponent::TouchDragComponent()
{
	m_lastFingerID = -1;
	SetName("TouchDrag");
}

TouchDragComponent::~TouchDragComponent()
{
}

void TouchDragComponent::OnAdd(Entity *pEnt)
{
	EntityComponent::OnAdd(pEnt);
	m_lastPos = CL_Vec2f(-1,-1);
	m_pDisabled = &GetVarWithDefault("disabled", uint32(0))->GetUINT32();
	m_pVisualStyle = &GetVarWithDefault("visualStyle", uint32(STYLE_NONE))->GetUINT32();
	m_pOnTouchDragUpdate = GetFunction("OnTouchDragUpdate");
	m_pPos2d = &GetParent()->GetVar("pos2d")->GetVector2();
	m_pSize2d = &GetParent()->GetVar("size2d")->GetVector2();
	m_pMult = &GetVarWithDefault("mult", CL_Vec2f(1,1))->GetVector2();
	m_pSwapXAndY = &GetVar("swapXAndY")->GetUINT32();
	m_pReverseX = &GetVar("reverseX")->GetUINT32();
	m_pReverseY = &GetVar("reverseY")->GetUINT32();

	//this will only be set if TouchDragComponent is initted before the TouchHandler...
	//GetParent()->GetVarWithDefault(string("touchPadding"), Variant(CL_Rectf(0.0f, 0.0f, 0.0f, 0.0f)))->GetRect();

	//register to get updated every frame

	GetParent()->GetFunction("OnInput")->sig_function.connect(1, boost::bind(&TouchDragComponent::OnInput, this, _1));

}

void TouchDragComponent::OnRemove()
{
	EntityComponent::OnRemove();
}

void TouchDragComponent::SetPosition(CL_Vec2f vInputPos)
{
	//if (vPos == m_lastPos) return;
	
	CL_Vec2f vPos = vInputPos-m_lastPos;

	m_lastPos = vInputPos;

	if (*m_pSwapXAndY != 0)
	{
		swap(vPos.x, vPos.y);
	}
	if (*m_pReverseX != 0)
	{
		vPos.x = 1-vPos.x;
	}
	if (*m_pReverseY != 0)
	{
		vPos.y = 1-vPos.y;
	}
	vPos.x *= m_pMult->x;
	vPos.y *= m_pMult->y;

	m_pOnTouchDragUpdate->sig_function(&VariantList(this, vPos));
}



void TouchDragComponent::OnInput( VariantList *pVList )
{
	//0 = message type, 1 = parent coordinate offset
	CL_Vec2f pt = pVList->Get(1).GetVector2();
	int fingerID = pVList->Get(2).GetUINT32();

	//LogMsg("Detected finger %d", fingerID);
	switch (eMessageType( int(pVList->Get(0).GetFloat())))
	{
	case MESSAGE_TYPE_GUI_CLICK_START:
		//first, determine if the click is on our area
		{
			CL_Rectf r(*m_pPos2d, CL_Sizef(m_pSize2d->x, m_pSize2d->y));

			if (r.contains(pt))
			{
				if (m_lastFingerID != -1)
				{
					LogMsg("Ignoring new finger..");
					return;
				}
				m_lastPos = pt;
				m_lastFingerID = fingerID;
			}
		}
		break;

	case MESSAGE_TYPE_GUI_CLICK_END:
		
		if (m_lastFingerID == fingerID)
		{
			m_lastFingerID = -1;
		}
		//HandleClickEnd(pt);
		break;
	case MESSAGE_TYPE_GUI_CLICK_MOVE:
		if (m_lastFingerID == fingerID)
		{
			SetPosition(pt);
		}
		//HandleClickMove(pt);
		break;
	}	

}