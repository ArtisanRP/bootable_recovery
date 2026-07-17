#ifndef _BATTERYPILL_HEADER
#define _BATTERYPILL_HEADER

#include "objects.hpp"
#include <string>

class GUIBatteryPill : public GUIObject, public RenderObject
{
public:
	GUIBatteryPill(xml_node<>* node);
	virtual ~GUIBatteryPill();

	virtual int Render(void);
	virtual int Update(void);
	virtual int NotifyVarChange(const std::string& varName, const std::string& value);

protected:
	void ParseBatteryValue(const std::string& value, int& level, bool& charging);
	void EnsureCaps(COLOR color);
	void FreeCaps();

	COLOR mBgColor;
	COLOR mFgColor;
	COLOR mFgCritical;
	COLOR mTextColor;
	COLOR mTextCritical;

	int mBatteryLevel;
	bool mCharging;
	bool mCritical;
	float mPulsePhase;
	int mRadius;
	void* mFont;
	bool mVarChanged;

	int mPillWidth;
	int mPillHeight;
	int mTextScale;

	gr_surface mLeftCap;
	gr_surface mRightCap;
	COLOR mCachedCapColor;
	bool mCapsValid;
	int mLastLevel;
	bool mLastCharging;
};

#endif
