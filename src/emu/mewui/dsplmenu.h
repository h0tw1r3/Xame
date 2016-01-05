// license:BSD-3-Clause
// copyright-holders:Dankan1890
/***************************************************************************

    mewui/dsplmenu.h

    MEWUI video options menu.

***************************************************************************/

#pragma once

#ifndef __MEWUI_DSPLMENU_H__
#define __MEWUI_DSPLMENU_H__

//-------------------------------------------------
//  class display options menu
//-------------------------------------------------
class ui_menu_display_options : public ui_menu
{
public:
	ui_menu_display_options(running_machine &machine, render_container *container);
	virtual ~ui_menu_display_options();
	virtual void populate() override;
	virtual void handle() override;
	virtual void custom_render(void *selectedref, float top, float bottom, float x, float y, float x2, float y2) override;

private:
	struct dspl_option
	{
		UINT16      status;
		const char  *description;
		const char  *option;
	};

	struct video_modes
	{
		const char  *option;
		const char  *label;
	};

	static video_modes m_video[];
	static dspl_option m_options[];
};

#endif /* __MEWUI_DSPLMENU_H__ */
