#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include <string>

extern "C" {
#include "../twcommon.h"
}

#include "minuitwrp/minui.h"
#include "minuitwrp/truetype.hpp"

#include "rapidxml.hpp"
#include "objects.hpp"

GUIBatteryPill::GUIBatteryPill(xml_node<>* node)
	: GUIObject(node)
{
	mFont = NULL;
	mBatteryLevel = 0;
	mCharging = false;
	mCritical = false;
	mPulsePhase = 0.0f;
	mRadius = 0;
	mPillWidth = 100;
	mPillHeight = 24;
	mTextScale = 100;
	mVarChanged = 1;
	mLeftCap = NULL;
	mRightCap = NULL;
	mCapsValid = false;
	mLastLevel = -1;
	mLastCharging = false;

	if (!node)
		return;

	mBgColor = LoadAttrColor(node, "color", COLOR(30, 30, 30, 200));
	mFgColor = LoadAttrColor(node, "fillcolor", COLOR(0, 144, 202, 255));
	mFgCritical = LoadAttrColor(node, "fillcolor_critical", COLOR(255, 30, 30, 255));
	mTextColor = LoadAttrColor(node, "textcolor", COLOR(255, 255, 255, 255));
	mTextCritical = LoadAttrColor(node, "textcolor_critical", COLOR(255, 255, 255, 255));

	mFont = LoadAttrFont(FindNode(node, "font"), "resource");

	xml_node<>* child;
	child = FindNode(node, "pillwidth");
	if (child && child->value()) {
		mPillWidth = atoi(child->value());
		if (mPillWidth < 10) mPillWidth = 10;
	}
	child = FindNode(node, "pillheight");
	if (child && child->value()) {
		mPillHeight = atoi(child->value());
		if (mPillHeight < 4) mPillHeight = 4;
	}
	child = FindNode(node, "textscale");
	if (child && child->value()) {
		mTextScale = atoi(child->value());
		if (mTextScale < 10) mTextScale = 10;
		if (mTextScale > 200) mTextScale = 200;
	}

	LoadPlacement(FindNode(node, "placement"), &mRenderX, &mRenderY, &mRenderW, &mRenderH, &mPlacement);

	mRadius = mPillHeight / 2;
}

GUIBatteryPill::~GUIBatteryPill()
{
	FreeCaps();
}

void GUIBatteryPill::FreeCaps()
{
	if (mLeftCap) {
		gr_free_surface(mLeftCap);
		mLeftCap = NULL;
	}
	if (mRightCap) {
		gr_free_surface(mRightCap);
		mRightCap = NULL;
	}
	mCapsValid = false;
}

void GUIBatteryPill::EnsureCaps(COLOR color)
{
	if (mCapsValid &&
		mCachedCapColor.red == color.red &&
		mCachedCapColor.green == color.green &&
		mCachedCapColor.blue == color.blue &&
		mCachedCapColor.alpha == color.alpha)
		return;

	FreeCaps();

	mLeftCap = gr_render_circle(mRadius, color.red, color.green, color.blue, color.alpha);
	mRightCap = gr_render_circle(mRadius, color.red, color.green, color.blue, color.alpha);
	mCachedCapColor = color;
	mCapsValid = true;
}

void GUIBatteryPill::ParseBatteryValue(const std::string& value, int& level, bool& charging)
{
	level = 0;
	charging = false;
	if (value.empty())
		return;
	size_t pct_pos = value.find('%');
	if (pct_pos == std::string::npos)
		return;
	std::string num = value.substr(0, pct_pos);
	level = atoi(num.c_str());
	if (level > 100) level = 101;
	if (level < 0) level = 0;
	if (pct_pos + 1 < value.size() && value[pct_pos + 1] == '+')
		charging = true;
}

int GUIBatteryPill::NotifyVarChange(const std::string& varName, const std::string& value)
{
	GUIObject::NotifyVarChange(varName, value);
	if (varName == "tw_battery") {
		mVarChanged = 1;
	}
	return 0;
}

int GUIBatteryPill::Update(void)
{
	if (!isConditionTrue())
		return 0;

	static int updateCounter = 0;
	if (updateCounter) {
		updateCounter--;
	} else {
		mVarChanged = 1;
		updateCounter = 3;
	}

	if (!mVarChanged && mLastLevel == mBatteryLevel && mLastCharging == mCharging)
		return 0;

	mVarChanged = 0;

	std::string batValue;
	DataManager::GetValue("tw_battery", batValue);
	ParseBatteryValue(batValue, mBatteryLevel, mCharging);
	mCritical = (mBatteryLevel <= 15 && mBatteryLevel >= 0);
	mCritical = mCritical && !mCharging;

	mLastLevel = mBatteryLevel;
	mLastCharging = mCharging;

	return 2;
}

int GUIBatteryPill::Render(void)
{
	if (!isConditionTrue())
		return 0;

	int x = mRenderX;
	int y = mRenderY;
	int w = mPillWidth;
	int h = mPillHeight;
	int r = mRadius;

	if (w < h)
		return -1;

	if (mCharging) {
		mPulsePhase += 0.08f;
		if (mPulsePhase > 6.2832f)
			mPulsePhase -= 6.2832f;
	}

	int innerW = w - 2 * r;

	COLOR fgColor = mCritical ? mFgCritical : mFgColor;

	if (mCharging) {
		float pulse = 0.78f + 0.22f * sinf(mPulsePhase);
		unsigned char pulseAlpha = (unsigned char)(fgColor.alpha * pulse);
		gr_color(fgColor.red, fgColor.green, fgColor.blue, pulseAlpha);
	} else {
		gr_color(fgColor.red, fgColor.green, fgColor.blue, fgColor.alpha);
	}

	int fillW = innerW * mBatteryLevel / 100;
	if (fillW < 0) fillW = 0;
	if (fillW > innerW) fillW = innerW;

	gr_color(mBgColor.red, mBgColor.green, mBgColor.blue, mBgColor.alpha);
	if (innerW > 0)
		gr_fill(x + r, y, innerW, h);

	EnsureCaps(mBgColor);
	if (mLeftCap)
		gr_blit(mLeftCap, 0, 0, r * 2 + 1, r * 2 + 1, x, y);
	if (mRightCap)
		gr_blit(mRightCap, 0, 0, r * 2 + 1, r * 2 + 1, x + w - r * 2, y);

	if (fillW > 0) {
		if (mCharging) {
			float pulse = 0.78f + 0.22f * sinf(mPulsePhase);
			unsigned char pulseAlpha = (unsigned char)(fgColor.alpha * pulse);
			COLOR pulseColor = fgColor;
			pulseColor.alpha = pulseAlpha;
			gr_color(pulseColor.red, pulseColor.green, pulseColor.blue, pulseColor.alpha);
		} else {
			gr_color(fgColor.red, fgColor.green, fgColor.blue, fgColor.alpha);
		}
		gr_fill(x + r, y, fillW, h);

		EnsureCaps(fgColor);
		if (fillW >= r && mLeftCap)
			gr_blit(mLeftCap, 0, 0, r * 2 + 1, r * 2 + 1, x, y);
		if (fillW >= innerW && mRightCap)
			gr_blit(mRightCap, 0, 0, r * 2 + 1, r * 2 + 1, x + w - r * 2, y);

		EnsureCaps(mBgColor);
	}

	COLOR textColor = mCritical ? mTextCritical : mTextColor;
	gr_color(textColor.red, textColor.green, textColor.blue, textColor.alpha);

	std::string text = std::to_string(mBatteryLevel) + "%";
	if (mCharging)
		text = "+" + text;

	int textX = x;
	int textW = w;
	if (mFont && mFont->GetResource()) {
		void* fontRes = mFont->GetResource();
		gr_textEx_scaleW(textX, y + (h / 2) - 2, text.c_str(), fontRes, textW, 4, mTextScale);
	}

	return 0;
}
