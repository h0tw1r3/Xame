// license:BSD-3-Clause
// copyright-holders:Dankan1890
/***************************************************************************

    mewui/menu.c

    Internal MEWUI menus for the user interface.

***************************************************************************/
#include "mewui/defimg.h"
#include "mewui/starimg.h"
#include "mewui/utils.h"
#include "mewui/optsmenu.h"
#include "mewui/datfile.h"
#include "rendfont.h"
#include "mewui/custmenu.h"
#include "mewui/icorender.h"

#define MAX_ICONS_RENDER         40

/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

render_texture *ui_menu::snapx_texture;
render_texture *ui_menu::hilight_main_texture;
render_texture *ui_menu::bgrnd_texture;
render_texture *ui_menu::star_texture;
render_texture *ui_menu::toolbar_texture[MEWUI_TOOLBAR_BUTTONS];
render_texture *ui_menu::sw_toolbar_texture[MEWUI_TOOLBAR_BUTTONS];
render_texture *ui_menu::icons_texture[MAX_ICONS_RENDER];
bitmap_argb32 *ui_menu::snapx_bitmap;
bitmap_argb32 *ui_menu::no_avail_bitmap;
bitmap_argb32 *ui_menu::star_bitmap;
bitmap_argb32 *ui_menu::bgrnd_bitmap;
bitmap_argb32 *ui_menu::icons_bitmap[MAX_ICONS_RENDER];
bitmap_rgb32 *ui_menu::hilight_main_bitmap;
bitmap_argb32 *ui_menu::toolbar_bitmap[MEWUI_TOOLBAR_BUTTONS];
bitmap_argb32 *ui_menu::sw_toolbar_bitmap[MEWUI_TOOLBAR_BUTTONS];

/***************************************************************************
    CONSTANTS
***************************************************************************/
struct ui_arts_info
{
	const char *title, *path, *addpath;
};

static const ui_arts_info arts_info[] =
{
	{ "Snapshots",       OPTION_SNAPSHOT_DIRECTORY,  "snap" },
	{ "Cabinets",        OPTION_CABINETS_PATH,       "cabinets;cabdevs" },
	{ "Control Panels",  OPTION_CPANELS_PATH,        "cpanel" },
	{ "PCBs",            OPTION_PCBS_PATH,           "pcb" },
	{ "Flyers",          OPTION_FLYERS_PATH,         "flyers" },
	{ "Titles",          OPTION_TITLES_PATH,         "titles" },
	{ "Ends",            OPTION_ENDS_PATH,           "ends" },
	{ "Artwork Preview", OPTION_ARTPREV_PATH,        "artwork preview" },
	{ "Bosses",          OPTION_BOSSES_PATH,         "bosses" },
	{ "Logos",           OPTION_LOGOS_PATH,          "logo" },
	{ "Versus",          OPTION_VERSUS_PATH,         "versus" },
	{ "Game Over",       OPTION_GAMEOVER_PATH,       "gameover" },
	{ "HowTo",           OPTION_HOWTO_PATH,          "howto" },
	{ "Scores",          OPTION_SCORES_PATH,         "scores" },
	{ "Select",          OPTION_SELECT_PATH,         "select" },
	{ "Marquees",        OPTION_MARQUEES_PATH,       "marquees" },
	{ NULL }
};

static const char *hover_msg[] = { "Add or remove favorites", "Export displayed list to file", "Show history.dat info",
                                           "Show mameinfo.dat / messinfo.dat info", "Show command.dat info", "Setup directories",
                                           "Configure options" };

//-------------------------------------------------
//  init - initialize the mewui menu system
//-------------------------------------------------

void ui_menu::init_mewui(running_machine &machine)
{
	// create a texture for hilighting items in main menu
	hilight_main_bitmap = auto_bitmap_rgb32_alloc(machine, 1, 26);
	int r1 = 0, g1 = 169, b1 = 255; //Any start color
	int r2 = 0, g2 = 39, b2 = 130; //Any stop color
	for (int y = 0; y < 26; y++)
	{
		int r = r1 + (y * (r2 - r1) / 26);
		int g = g1 + (y * (g2 - g1) / 26);
		int b = b1 + (y * (b2 - b1) / 26);
		hilight_main_bitmap->pix32(y, 0) = rgb_t(0xff, r, g, b);
	}

	hilight_main_texture = machine.render().texture_alloc();
	hilight_main_texture->set_bitmap(*hilight_main_bitmap, hilight_main_bitmap->cliprect(), TEXFORMAT_ARGB32);

	// create a texture for snapshot
	snapx_bitmap = auto_alloc(machine, bitmap_argb32);
	snapx_texture = machine.render().texture_alloc(render_texture::hq_scale);

	// allocates and sets the default "no available" image
	no_avail_bitmap = auto_alloc(machine, bitmap_argb32(256, 256));
	UINT32 *dst = &no_avail_bitmap->pix32(0);
	memcpy(dst, no_avail_bmp, 256*256*sizeof(UINT32));

	// allocates and sets the favorites star image
	star_bitmap = auto_alloc(machine, bitmap_argb32(32, 32));
	dst = &star_bitmap->pix32(0);
	memcpy(dst, favorite_star_bmp, 32*32*sizeof(UINT32));
	star_texture = machine.render().texture_alloc();
	star_texture->set_bitmap(*star_bitmap, star_bitmap->cliprect(), TEXFORMAT_ARGB32);

	// allocates icons bitmap and texture
	for (int i = 0; i < MAX_ICONS_RENDER; i++)
	{
		icons_bitmap[i] = auto_alloc(machine, bitmap_argb32(32, 32));
		icons_texture[i] = machine.render().texture_alloc(render_texture::hq_scale);
	}

	// create a texture for main menu background
	bgrnd_bitmap = auto_alloc(machine, bitmap_argb32);
	bgrnd_texture = machine.render().texture_alloc(render_texture::hq_scale);

	if (machine.options().use_background_image() && (machine.options().system() == &GAME_NAME(___empty) || machine.options().system() == NULL))
	{
		emu_file backgroundfile(".", OPEN_FLAG_READ);
		render_load_jpeg(*bgrnd_bitmap, backgroundfile, NULL, "background.jpg");

		if (!bgrnd_bitmap->valid())
			render_load_png(*bgrnd_bitmap, backgroundfile, NULL, "background.png");

		if (bgrnd_bitmap->valid())
			bgrnd_texture->set_bitmap(*bgrnd_bitmap, bgrnd_bitmap->cliprect(), TEXFORMAT_ARGB32);
		else
			bgrnd_bitmap->reset();
	}
	else
		bgrnd_bitmap->reset();

	// create a texture for toolbar
	emu_file toolfile(machine.options().mewui_path(), OPEN_FLAG_READ);
	for (int x = 0; x < MEWUI_TOOLBAR_BUTTONS; ++x)
	{
		toolbar_bitmap[x] = auto_alloc(machine, bitmap_argb32(32, 32));
		toolbar_texture[x] = machine.render().texture_alloc();
		UINT32 *dst = &toolbar_bitmap[x]->pix32(0);
		memcpy(dst, toolbar_bitmap_bmp[x], 32 * 32 * sizeof(UINT32));
		if (toolbar_bitmap[x]->valid())
			toolbar_texture[x]->set_bitmap(*toolbar_bitmap[x], toolbar_bitmap[x]->cliprect(), TEXFORMAT_ARGB32);
		else
			toolbar_bitmap[x]->reset();
	}

	// create a texture for toolbar
	for (int x = 0; x < MEWUI_TOOLBAR_BUTTONS; ++x)
	{
		sw_toolbar_bitmap[x] = auto_alloc(machine, bitmap_argb32(32, 32));
		sw_toolbar_texture[x] = machine.render().texture_alloc();
		UINT32 *dst;
		if (x == 0 || x == 2)
		{
			dst = &sw_toolbar_bitmap[x]->pix32(0);
			memcpy(dst, toolbar_bitmap_bmp[x], 32 * 32 * sizeof(UINT32));
			sw_toolbar_texture[x]->set_bitmap(*sw_toolbar_bitmap[x], sw_toolbar_bitmap[x]->cliprect(), TEXFORMAT_ARGB32);
		}
		else
			sw_toolbar_bitmap[x]->reset();
	}
}


//-------------------------------------------------
//  draw main menu
//-------------------------------------------------

void ui_menu::draw_select_game(bool noinput)
{
	float line_height = machine().ui().get_line_height();
	float ud_arrow_width = line_height * machine().render().ui_aspect();
	float gutter_width = 0.4f * line_height * machine().render().ui_aspect() * 1.3f;
	mouse_x = -1, mouse_y = -1;
	float right_panel_size = (mewui_globals::panels_status == HIDE_BOTH || mewui_globals::panels_status == HIDE_RIGHT_PANEL) ? 2.0f * UI_BOX_LR_BORDER: 0.3f;
	float visible_width = 1.0f - 4.0f * UI_BOX_LR_BORDER;
	float primary_left = (1.0f - visible_width) * 0.5f;
	float primary_width = visible_width;
	bool is_swlist = ((item[0].flags & MENU_FLAG_MEWUI_SWLIST) != 0);
	bool is_favorites = ((item[0].flags & MENU_FLAG_MEWUI_FAVORITE) != 0);

	// draw background image if available
	if (machine().options().use_background_image() && bgrnd_bitmap->valid())
		container->add_quad(0.0f, 0.0f, 1.0f, 1.0f, ARGB_WHITE, bgrnd_texture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));

	hover = item.size() + 1;
	visible_items = (is_swlist) ? item.size() - 2 : item.size() - 4;
	float extra_height = (is_swlist) ? 2.0f * line_height : 4.0f * line_height;
	float visible_extra_menu_height = customtop + custombottom + extra_height;

	// locate mouse
	mouse_hit = FALSE;
	mouse_button = FALSE;
	if (!noinput)
	{
		mouse_target = ui_input_find_mouse(machine(), &mouse_target_x, &mouse_target_y, &mouse_button);
		if (mouse_target != NULL)
			if (mouse_target->map_point_container(mouse_target_x, mouse_target_y, *container, mouse_x, mouse_y))
				mouse_hit = TRUE;
	}

	// account for extra space at the top and bottom
	float visible_main_menu_height = 1.0f - 2.0f * UI_BOX_TB_BORDER - visible_extra_menu_height;
	visible_lines = floor(visible_main_menu_height / line_height);
	visible_main_menu_height = (float)(visible_lines * line_height);

	if (!is_swlist)
		mewui_globals::visible_main_lines = visible_lines;
	else
		mewui_globals::visible_sw_lines = visible_lines;

	// compute top/left of inner menu area by centering
	float visible_left = primary_left;
	float visible_top = (1.0f - (visible_main_menu_height + visible_extra_menu_height)) * 0.5f;

	// if the menu is at the bottom of the extra, adjust
	visible_top += customtop;

	// compute left box size
	float x1 = visible_left - UI_BOX_LR_BORDER;
	float y1 = visible_top - UI_BOX_TB_BORDER;
	float x2 = x1 + 2.0f * UI_BOX_LR_BORDER;
	float y2 = visible_top + visible_main_menu_height + UI_BOX_TB_BORDER + extra_height;

	// add left box
	visible_left = draw_left_panel(x1, y1, x2, y2);
	visible_width -= right_panel_size + visible_left - 2.0f * UI_BOX_LR_BORDER;

	// compute and add main box
	x1 = visible_left - UI_BOX_LR_BORDER;
	x2 = visible_left + visible_width + UI_BOX_LR_BORDER;
	float line = visible_top + (float)(visible_lines * line_height);

	//machine().ui().draw_outlined_box(container, x1, y1, x2, y2, rgb_t(0xEF, 0x12, 0x47, 0x7B));
	machine().ui().draw_outlined_box(container, x1, y1, x2, y2, UI_BACKGROUND_COLOR);

	if (visible_items < visible_lines)
		visible_lines = visible_items;
	if (top_line < 0 || selected == 0)
		top_line = 0;
	if (selected < visible_items && top_line + visible_lines >= visible_items)
		top_line = visible_items - visible_lines;

	// determine effective positions taking into account the hilighting arrows
	float effective_width = visible_width - 2.0f * gutter_width;
	float effective_left = visible_left + gutter_width;

	int n_loop = (visible_items >= visible_lines) ? visible_lines : visible_items;

	for (int linenum = 0; linenum < n_loop; linenum++)
	{
		float line_y = visible_top + (float)linenum * line_height;
		int itemnum = top_line + linenum;
		const ui_menu_item &pitem = item[itemnum];
		const char *itemtext = pitem.text;
		rgb_t fgcolor = UI_TEXT_COLOR;
		rgb_t bgcolor = UI_TEXT_BG_COLOR;
		rgb_t fgcolor3 = UI_CLONE_COLOR;
		float line_x0 = x1 + 0.5f * UI_LINE_WIDTH;
		float line_y0 = line_y;
		float line_x1 = x2 - 0.5f * UI_LINE_WIDTH;
		float line_y1 = line_y + line_height;

		// set the hover if this is our item
		if (mouse_hit && line_x0 <= mouse_x && line_x1 > mouse_x && line_y0 <= mouse_y && line_y1 > mouse_y && pitem.is_selectable())
			hover = itemnum;

		// if we're selected, draw with a different background
		if (itemnum == selected)
		{
			fgcolor = rgb_t(0xff, 0xff, 0xff, 0x00);
			bgcolor = rgb_t(0xff, 0xff, 0xff, 0xff);
			fgcolor3 = rgb_t(0xff, 0xcc, 0xcc, 0x00);
		}
		// else if the mouse is over this item, draw with a different background
		else if (itemnum == hover)
		{
			fgcolor = UI_MOUSEOVER_COLOR;
			bgcolor = UI_MOUSEOVER_BG_COLOR;
			fgcolor3 = UI_MOUSEOVER_COLOR;
		}

		// if we have some background hilighting to do, add a quad behind everything else
		if (bgcolor != UI_TEXT_BG_COLOR)
			machine().ui().draw_textured_box(container, line_x0 + 0.01f, line_y0, line_x1 - 0.01f, line_y1,
			                                 bgcolor, rgb_t(255, 43, 43, 43), hilight_main_texture,
			                                 PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXWRAP(TRUE));

		// if we're on the top line, display the up arrow
		if (linenum == 0 && top_line != 0)
		{
			draw_arrow(container, 0.5f * (x1 + x2) - 0.5f * ud_arrow_width, line_y + 0.25f * line_height,
			           0.5f * (x1 + x2) + 0.5f * ud_arrow_width, line_y + 0.75f * line_height, fgcolor, ROT0);

			if (hover == itemnum)
				hover = HOVER_ARROW_UP;
		}
		// if we're on the bottom line, display the down arrow
		else if (linenum == visible_lines - 1 && itemnum != visible_items - 1)
		{
			draw_arrow(container, 0.5f * (x1 + x2) - 0.5f * ud_arrow_width, line_y + 0.25f * line_height,
						0.5f * (x1 + x2) + 0.5f * ud_arrow_width, line_y + 0.75f * line_height, fgcolor, ROT0 ^ ORIENTATION_FLIP_Y);

			if (hover == itemnum)
				hover = HOVER_ARROW_DOWN;
		}
		// if we're just a divider, draw a line
		else if (strcmp(itemtext, MENU_SEPARATOR_ITEM) == 0)
			container->add_line(visible_left, line_y + 0.5f * line_height, visible_left + visible_width, line_y + 0.5f * line_height,
			                    UI_LINE_WIDTH, UI_TEXT_COLOR, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
		// draw the item centered
		else if (pitem.subtext == NULL)
		{
			int item_invert = pitem.flags & MENU_FLAG_INVERT;
			float space = 0.0f;

			if (!is_swlist)
			{
				if (is_favorites)
				{
					ui_software_info *soft = (ui_software_info *)item[itemnum].ref;
					if (soft->startempty == 1)
						draw_icon(container, linenum, (void *)soft->driver, effective_left, line_y);
				}
				else
					draw_icon(container, linenum, item[itemnum].ref, effective_left, line_y);

				space = machine().ui().get_line_height() * container->manager().ui_aspect() * 1.5f;
			}
			machine().ui().draw_text_full(container, itemtext, effective_left + space, line_y, effective_width - space,
			                              JUSTIFY_LEFT, WRAP_TRUNCATE, DRAW_NORMAL, item_invert ? fgcolor3 : fgcolor,
			                              bgcolor, NULL, NULL);
		}
		else
		{
			int item_invert = pitem.flags & MENU_FLAG_INVERT;
			const char *subitem_text = pitem.subtext;
			float item_width, subitem_width;

			// compute right space for subitem
			machine().ui().draw_text_full(container, subitem_text, effective_left, line_y, machine().ui().get_string_width(pitem.subtext),
			                              JUSTIFY_RIGHT, WRAP_NEVER, DRAW_NONE, item_invert ? fgcolor3 : fgcolor, bgcolor, &subitem_width, NULL);
			subitem_width += gutter_width;

			// draw the item left-justified
			machine().ui().draw_text_full(container, itemtext, effective_left, line_y, effective_width - subitem_width,
			                              JUSTIFY_LEFT, WRAP_TRUNCATE, DRAW_NORMAL, item_invert ? fgcolor3 : fgcolor, bgcolor, &item_width, NULL);

			// draw the subitem right-justified
			machine().ui().draw_text_full(container, subitem_text, effective_left + item_width, line_y, effective_width - item_width,
			                              JUSTIFY_RIGHT, WRAP_NEVER, DRAW_NORMAL, item_invert ? fgcolor3 : fgcolor, bgcolor, NULL, NULL);
		}
	}

	for (size_t count = visible_items; count < item.size(); count++)
	{
		const ui_menu_item &pitem = item[count];
		const char *itemtext = pitem.text;
		float line_x0 = x1 + 0.5f * UI_LINE_WIDTH;
		float line_y0 = line;
		float line_x1 = x2 - 0.5f * UI_LINE_WIDTH;
		float line_y1 = line + line_height;
		rgb_t fgcolor = UI_TEXT_COLOR;
		rgb_t bgcolor = UI_TEXT_BG_COLOR;

		if (mouse_hit && line_x0 <= mouse_x && line_x1 > mouse_x && line_y0 <= mouse_y && line_y1 > mouse_y && pitem.is_selectable())
			hover = count;

		// if we're selected, draw with a different background
		if (count == selected)
		{
			fgcolor = rgb_t(0xff, 0xff, 0xff, 0x00);
			bgcolor = rgb_t(0xff, 0xff, 0xff, 0xff);
		}
		// else if the mouse is over this item, draw with a different background
		else if (count == hover)
		{
			fgcolor = UI_MOUSEOVER_COLOR;
			bgcolor = UI_MOUSEOVER_BG_COLOR;
		}

		// if we have some background hilighting to do, add a quad behind everything else
		if (bgcolor != UI_TEXT_BG_COLOR)
			machine().ui().draw_textured_box(container, line_x0 + 0.01f, line_y0, line_x1 - 0.01f, line_y1, bgcolor, rgb_t(255, 43, 43, 43),
			                                 hilight_main_texture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXWRAP(TRUE));

		if (strcmp(itemtext, MENU_SEPARATOR_ITEM) == 0)
			container->add_line(visible_left, line + 0.5f * line_height, visible_left + visible_width, line + 0.5f * line_height,
			                    UI_LINE_WIDTH, UI_TEXT_COLOR, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
		else
			machine().ui().draw_text_full(container, itemtext, effective_left, line, effective_width,
			                              JUSTIFY_CENTER, WRAP_TRUNCATE, DRAW_NORMAL, fgcolor, bgcolor, NULL, NULL);
		line += line_height;
	}

	x1 = x2;
	x2 += right_panel_size;

	draw_right_panel((selected >= 0 && selected < item.size()) ? item[selected].ref : NULL, x1, y1, x2, y2);

	x1 = primary_left - UI_BOX_LR_BORDER;
	x2 = primary_left + primary_width + UI_BOX_LR_BORDER;

	// if there is something special to add, do it by calling the virtual method
	custom_render((selected >= 0 && selected < item.size()) ? item[selected].ref : NULL, customtop, custombottom, x1, y1, x2, y2);

	// return the number of visible lines, minus 1 for top arrow and 1 for bottom arrow
	visitems = visible_lines - (top_line != 0) - (top_line + visible_lines != visible_items);

	// reset redraw icon stage
	if (!is_swlist)
		mewui_globals::redraw_icon = false;
}

//-------------------------------------------------
//  get title and search path for right panel
//-------------------------------------------------

void ui_menu::get_title_search(std::string &snaptext, std::string &searchstr)
{
	// get arts title text
	snaptext.assign(arts_info[mewui_globals::curimage_view].title);

	// get search path
	path_iterator path(machine().options().value(arts_info[mewui_globals::curimage_view].path));
	std::string curpath;
	searchstr.assign(machine().options().value(arts_info[mewui_globals::curimage_view].path));

	// iterate over path and add path for zipped formats
	while (path.next(curpath))
	{
		path_iterator path_iter(arts_info[mewui_globals::curimage_view].addpath);
		std::string c_path;
		while (path_iter.next(c_path))
			searchstr.append(";").append(curpath).append(PATH_SEPARATOR).append(c_path);
	}
}

//-------------------------------------------------
//  handle keys for main menu
//-------------------------------------------------

void ui_menu::handle_main_keys(UINT32 flags)
{
	bool ignorepause = ui_menu::stack_has_special_main_menu();

	// bail if no items
	if (item.size() == 0)
		return;

	// if we hit select, return TRUE or pop the stack, depending on the item
	if (exclusive_input_pressed(IPT_UI_SELECT, 0))
	{
		if (selected == item.size() - 1)
		{
			menu_event.iptkey = IPT_UI_CANCEL;
			ui_menu::stack_pop(machine());
		}
		return;
	}

	// hitting cancel also pops the stack
	if (exclusive_input_pressed(IPT_UI_CANCEL, 0))
	{
		if (!menu_has_search_active())
			ui_menu::stack_pop(machine());
//    else if (!ui_error)
//        ui_menu::stack_pop(machine()); TODO
		return;
	}

	// validate the current selection
	validate_selection(1);

	// swallow left/right keys if they are not appropriate
	bool ignoreleft = ((item[selected].flags & MENU_FLAG_LEFT_ARROW) == 0 || mewui_globals::panels_status == HIDE_BOTH || mewui_globals::panels_status == HIDE_RIGHT_PANEL);
	bool ignoreright = ((item[selected].flags & MENU_FLAG_RIGHT_ARROW) == 0 || mewui_globals::panels_status == HIDE_BOTH || mewui_globals::panels_status == HIDE_RIGHT_PANEL);
	bool ignoreup = (mewui_globals::panels_status == HIDE_BOTH || mewui_globals::panels_status == HIDE_LEFT_PANEL);
	bool ignoredown = (mewui_globals::panels_status == HIDE_BOTH || mewui_globals::panels_status == HIDE_LEFT_PANEL);

	// accept left/right keys as-is with repeat
	if (!ignoreleft && exclusive_input_pressed(IPT_UI_LEFT, (flags & UI_MENU_PROCESS_LR_REPEAT) ? 6 : 0))
	{
		// Swap the right panel
		if (machine().input().code_pressed(KEYCODE_LCONTROL) || machine().input().code_pressed(JOYCODE_BUTTON1))
			menu_event.iptkey = IPT_UI_LEFT_PANEL;
		return;
	}

	if (!ignoreright && exclusive_input_pressed(IPT_UI_RIGHT, (flags & UI_MENU_PROCESS_LR_REPEAT) ? 6 : 0))
	{
		// Swap the right panel
		if (machine().input().code_pressed(KEYCODE_LCONTROL) || machine().input().code_pressed(JOYCODE_BUTTON1))
			menu_event.iptkey = IPT_UI_RIGHT_PANEL;
		return;
	}

	// up backs up by one item
	if (exclusive_input_pressed(IPT_UI_UP, 6))
	{
		// Filter
		if (!ignoreup && (machine().input().code_pressed(KEYCODE_LALT) || machine().input().code_pressed(JOYCODE_BUTTON2)))
		{
			menu_event.iptkey = IPT_UI_UP_FILTER;
			return;
		}

		// Infos
		if (!ignoreleft && (machine().input().code_pressed(KEYCODE_LCONTROL) || machine().input().code_pressed(JOYCODE_BUTTON1)))
		{
			menu_event.iptkey = IPT_UI_UP_PANEL;
			topline_datsview--;
			return;
		}

		if (selected == visible_items + 1 || selected == 0 || ui_error)
			return;

		selected--;

		if (selected == top_line && top_line != 0)
			top_line--;
	}

	// down advances by one item
	if (exclusive_input_pressed(IPT_UI_DOWN, 6))
	{
		// Filter
		if (!ignoredown && (machine().input().code_pressed(KEYCODE_LALT) || machine().input().code_pressed(JOYCODE_BUTTON2)))
		{
			menu_event.iptkey = IPT_UI_DOWN_FILTER;
			return;
		}

		// Infos
		if (!ignoreright && (machine().input().code_pressed(KEYCODE_LCONTROL) || machine().input().code_pressed(JOYCODE_BUTTON1)))
		{
			menu_event.iptkey = IPT_UI_DOWN_PANEL;
			topline_datsview++;
			return;
		}

		if (selected == item.size() - 1 || selected == visible_items - 1 || ui_error)
			return;

		selected++;

		if (selected == top_line + visitems + (top_line != 0))
			top_line++;
	}

	// page up backs up by visitems
	if (exclusive_input_pressed(IPT_UI_PAGE_UP, 6))
	{
		// Infos
		if (!ignoreleft && (machine().input().code_pressed(KEYCODE_LCONTROL) || machine().input().code_pressed(JOYCODE_BUTTON1)))
		{
			menu_event.iptkey = IPT_UI_DOWN_PANEL;
			topline_datsview -= right_visible_lines - 1;
			return;
		}

		if (selected < visible_items && !ui_error)
		{
			selected -= visitems;

			if (selected < 0)
				selected = 0;

			top_line -= visitems - (top_line + visible_lines == visible_items);
		}
	}

	// page down advances by visitems
	if (exclusive_input_pressed(IPT_UI_PAGE_DOWN, 6))
	{
		// Infos
		if (!ignoreleft && (machine().input().code_pressed(KEYCODE_LCONTROL) || machine().input().code_pressed(JOYCODE_BUTTON1)))
		{
			menu_event.iptkey = IPT_UI_DOWN_PANEL;
			topline_datsview += right_visible_lines - 1;
			return;
		}

		if (selected < visible_items && !ui_error)
		{
			selected += visible_lines - 2 + (selected == 0);

			if (selected >= visible_items)
				selected = visible_items - 1;

			top_line += visible_lines - 2;
		}
	}

	// home goes to the start
	if (exclusive_input_pressed(IPT_UI_HOME, 0))
	{
		// Infos
		if (!ignoreleft && (machine().input().code_pressed(KEYCODE_LCONTROL) || machine().input().code_pressed(JOYCODE_BUTTON1)))
		{
			menu_event.iptkey = IPT_UI_DOWN_PANEL;
			topline_datsview = 0;
			return;
		}

		if (selected < visible_items && !ui_error)
		{
			selected = 0;
			top_line = 0;
		}
	}

	// end goes to the last
	if (exclusive_input_pressed(IPT_UI_END, 0))
	{
		// Infos
		if (!ignoreleft && (machine().input().code_pressed(KEYCODE_LCONTROL) || machine().input().code_pressed(JOYCODE_BUTTON1)))
		{
			menu_event.iptkey = IPT_UI_DOWN_PANEL;
			topline_datsview = totallines;
			return;
		}

		if (selected < visible_items && !ui_error)
			selected = top_line = visible_items - 1;
	}

	// pause enables/disables pause
	if (!ui_error && !ignorepause && exclusive_input_pressed(IPT_UI_PAUSE, 0))
	{
		if (machine().paused())
			machine().resume();
		else
			machine().pause();
	}

	// handle a toggle cheats request
	if (!ui_error && ui_input_pressed_repeat(machine(), IPT_UI_TOGGLE_CHEAT, 0))
		machine().cheat().set_enable(!machine().cheat().enabled());

	// see if any other UI keys are pressed
	if (menu_event.iptkey == IPT_INVALID)
		for (int code = IPT_UI_FIRST + 1; code < IPT_UI_LAST; code++)
		{
			if (ui_error || code == IPT_UI_CONFIGURE || (code == IPT_UI_LEFT && ignoreleft)
				|| (code == IPT_UI_RIGHT && ignoreright) || (code == IPT_UI_PAUSE && ignorepause))
				continue;

			if (exclusive_input_pressed(code, 0))
				break;
		}
}

//-------------------------------------------------
//  handle input events for main menu
//-------------------------------------------------

void ui_menu::handle_main_events(UINT32 flags)
{
	int stop = false;
	ui_event local_menu_event;

	// loop while we have interesting events
	while (!stop && ui_input_pop_event(machine(), &local_menu_event))
	{
		switch (local_menu_event.event_type)
		{
			// if we are hovering over a valid item, select it with a single click
			case UI_EVENT_MOUSE_DOWN:
			{
				if (ui_error)
				{
					menu_event.iptkey = IPT_OTHER;
					stop = true;
				}
				else
				{
					if (hover >= 0 && hover < item.size())
						selected = hover;
					else if (hover == HOVER_ARROW_UP)
					{
						selected -= visitems;
						if (selected < 0)
							selected = 0;
						top_line -= visitems - (top_line + visible_lines == visible_items);
					}
					else if (hover == HOVER_ARROW_DOWN)
					{
						selected += visible_lines - 2 + (selected == 0);
						if (selected >= visible_items)
							selected = visible_items - 1;
						top_line += visible_lines - 2;
					}
					else if (hover == HOVER_UI_RIGHT)
						menu_event.iptkey = IPT_UI_RIGHT;
					else if (hover == HOVER_UI_LEFT)
						menu_event.iptkey = IPT_UI_LEFT;
					else if (hover == HOVER_DAT_DOWN)
						topline_datsview += right_visible_lines - 1;
					else if (hover == HOVER_DAT_UP)
						topline_datsview -= right_visible_lines - 1;
					else if (hover == HOVER_LPANEL_ARROW)
					{
						if (mewui_globals::panels_status == HIDE_LEFT_PANEL)
							mewui_globals::panels_status = SHOW_PANELS;
						else if (mewui_globals::panels_status == HIDE_BOTH)
							mewui_globals::panels_status = HIDE_RIGHT_PANEL;
						else if (mewui_globals::panels_status == SHOW_PANELS)
							mewui_globals::panels_status = HIDE_LEFT_PANEL;
						else if (mewui_globals::panels_status == HIDE_RIGHT_PANEL)
							mewui_globals::panels_status = HIDE_BOTH;
					}
					else if (hover == HOVER_RPANEL_ARROW)
					{
						if (mewui_globals::panels_status == HIDE_RIGHT_PANEL)
							mewui_globals::panels_status = SHOW_PANELS;
						else if (mewui_globals::panels_status == HIDE_BOTH)
							mewui_globals::panels_status = HIDE_LEFT_PANEL;
						else if (mewui_globals::panels_status == SHOW_PANELS)
							mewui_globals::panels_status = HIDE_RIGHT_PANEL;
						else if (mewui_globals::panels_status == HIDE_LEFT_PANEL)
							mewui_globals::panels_status = HIDE_BOTH;
					}
					else if (hover == HOVER_B_FAV)
					{
						menu_event.iptkey = IPT_UI_FAVORITES;
						stop = true;
					}
					else if (hover == HOVER_B_EXPORT)
					{
						menu_event.iptkey = IPT_UI_EXPORT;
						stop = true;
					}
					else if (hover == HOVER_B_HISTORY)
					{
						menu_event.iptkey = IPT_UI_HISTORY;
						stop = true;
					}
					else if (hover == HOVER_B_MAMEINFO)
					{
						menu_event.iptkey = IPT_UI_MAMEINFO;
						stop = true;
					}
					else if (hover == HOVER_B_COMMAND)
					{
						menu_event.iptkey = IPT_UI_COMMAND;
						stop = true;
					}
					else if (hover == HOVER_B_SETTINGS)
					{
						menu_event.iptkey = IPT_UI_SELECT;
						selected = visible_items + 1;
						stop = true;
					}
					else if (hover == HOVER_B_FOLDERS)
					{
						menu_event.iptkey = IPT_UI_SELECT;
						selected = visible_items + 2;
						stop = true;
					}
					else if (hover >= HOVER_MAME_ALL && hover <= HOVER_MAME_SYSTEMS)
					{
						ume_filters::actual = (HOVER_MAME_ALL - hover) * (-1);
						menu_event.iptkey = IPT_OTHER;
						stop = true;
					}
					else if (hover >= HOVER_RP_FIRST && hover <= HOVER_RP_LAST)
					{
						mewui_globals::rpanel = (HOVER_RP_FIRST - hover) * (-1);
						stop = true;
					}
					else if (hover >= HOVER_SW_FILTER_FIRST && hover <= HOVER_SW_FILTER_LAST)
					{
						l_sw_hover = (HOVER_SW_FILTER_FIRST - hover) * (-1);
						menu_event.iptkey = IPT_OTHER;
						stop = true;
					}
					else if (hover >= HOVER_FILTER_FIRST && hover <= HOVER_FILTER_LAST)
					{
						l_hover = (HOVER_FILTER_FIRST - hover) * (-1);
						menu_event.iptkey = IPT_OTHER;
						stop = true;
					}
				}
				break;
			}

			// if we are hovering over a valid item, fake a UI_SELECT with a double-click
			case UI_EVENT_MOUSE_DOUBLE_CLICK:
				if (hover >= 0 && hover < item.size())
				{
					selected = hover;
					menu_event.iptkey = IPT_UI_SELECT;
				}

				if (selected == item.size() - 1)
				{
					menu_event.iptkey = IPT_UI_CANCEL;
					ui_menu::stack_pop(machine());
				}
				stop = true;
				break;

			// caught scroll event
			case UI_EVENT_MOUSE_SCROLL:
				if (local_menu_event.zdelta > 0)
				{
					if (selected >= visible_items || selected == 0 || ui_error)
						break;
					selected -= local_menu_event.num_lines;
					if (selected < top_line + (top_line != 0))
						top_line -= local_menu_event.num_lines;
				}
				else
				{
					if (selected >= visible_items - 1 || ui_error)
						break;
					selected += local_menu_event.num_lines;
					if (selected > visible_items - 1)
						selected = visible_items - 1;
					if (selected >= top_line + visitems + (top_line != 0))
						top_line += local_menu_event.num_lines;
				}
				break;

			// translate CHAR events into specials
			case UI_EVENT_CHAR:
				menu_event.iptkey = IPT_SPECIAL;
				menu_event.unichar = local_menu_event.ch;
				stop = true;
				break;

			// ignore everything else
			default:
				break;
		}
	}
}

//-------------------------------------------------
//  draw UME box
//-------------------------------------------------

void ui_menu::draw_ume_box(float x1, float y1, float x2, float y2)
{
	float text_size = 0.65f;
	float line_height = machine().ui().get_line_height() * text_size;
	float maxwidth = 0.0f;

	for (int x = 0; x < ume_filters::length; x++)
	{
		float width;
		// compute width of left hand side
		machine().ui().draw_text_full(container, ume_filters::text[x], 0.0f, 0.0f, 1.0f, JUSTIFY_CENTER, WRAP_NEVER,
		                              DRAW_NONE, UI_TEXT_COLOR, ARGB_BLACK, &width, NULL, text_size);
		width += 2 * UI_BOX_LR_BORDER;
		maxwidth = MAX(maxwidth, width);
	}

	x2 = x1 + maxwidth;

	machine().ui().draw_outlined_box(container, x1, y1, x2, y2, UI_BACKGROUND_COLOR);

	// take off the borders
	x1 += UI_BOX_LR_BORDER;
	x2 -= UI_BOX_LR_BORDER;
	y1 += UI_BOX_TB_BORDER;
	y2 -= UI_BOX_TB_BORDER;

	for (int filter = 0; filter < ume_filters::length; filter++)
	{
		rgb_t bgcolor = UI_TEXT_BG_COLOR;
		rgb_t fgcolor = UI_TEXT_COLOR;

		if (mouse_hit && x1 <= mouse_x && x2 > mouse_x && y1 <= mouse_y && y1 + line_height > mouse_y)
		{
			bgcolor = UI_MOUSEOVER_BG_COLOR;
			fgcolor = UI_MOUSEOVER_COLOR;
			hover = HOVER_MAME_ALL + filter;
		}

		if (ume_filters::actual == filter)
		{
			bgcolor = UI_SELECTED_BG_COLOR;
			fgcolor = UI_SELECTED_COLOR;
		}

		if (bgcolor != UI_TEXT_BG_COLOR)
			container->add_rect(x1, y1, x2, y1 + line_height, bgcolor, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXWRAP(TRUE));

		machine().ui().draw_text_full(container, ume_filters::text[filter], x1, y1, x2 - x1, JUSTIFY_CENTER, WRAP_NEVER,
		                              DRAW_NORMAL, fgcolor, bgcolor, NULL, NULL, text_size);

		y1 += line_height;
	}
}

//-------------------------------------------------
//  draw right box title
//-------------------------------------------------

float ui_menu::draw_right_box_title(float x1, float y1, float x2, float y2)
{
	float line_height = machine().ui().get_line_height();
	float midl = (x2 - x1) * 0.5f;

	// add outlined box for options
	//machine().ui().draw_outlined_box(container, x1, y1, x2, y2, rgb_t(0xEF, 0x12, 0x47, 0x7B));
	machine().ui().draw_outlined_box(container, x1, y1, x2, y2, UI_BACKGROUND_COLOR);

	// add separator line
	container->add_line(x1 + midl, y1, x1 + midl, y1 + line_height, UI_LINE_WIDTH, UI_BORDER_COLOR, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));

	std::string buffer[RP_LAST + 1];
	buffer[RP_IMAGES].assign("Images");
	buffer[RP_INFOS].assign("Infos");

	for (int cells = RP_IMAGES; cells <= RP_INFOS; cells++)
	{
		rgb_t bgcolor = UI_TEXT_BG_COLOR;
		rgb_t fgcolor = UI_TEXT_COLOR;

		if (mouse_hit && x1 <= mouse_x && x1 + midl > mouse_x && y1 <= mouse_y && y1 + line_height > mouse_y)
		{
			if (mewui_globals::rpanel != cells)
			{
				bgcolor = UI_MOUSEOVER_BG_COLOR;
				fgcolor = UI_MOUSEOVER_COLOR;
				hover = HOVER_RP_FIRST + cells;
			}
		}

		if (mewui_globals::rpanel != cells)
		{
			container->add_line(x1, y1 + line_height, x1 + midl, y1 + line_height, UI_LINE_WIDTH,
			                    UI_BORDER_COLOR, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
			if (fgcolor != UI_MOUSEOVER_COLOR)
				fgcolor = UI_CLONE_COLOR;
		}

		if (bgcolor != UI_TEXT_BG_COLOR)
			container->add_rect(x1 + UI_LINE_WIDTH, y1 + UI_LINE_WIDTH, x1 + midl - UI_LINE_WIDTH, y1 + line_height,
			                    bgcolor, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXWRAP(TRUE));

		machine().ui().draw_text_full(container, buffer[cells].c_str(), x1 + UI_LINE_WIDTH, y1, midl - UI_LINE_WIDTH,
		                              JUSTIFY_CENTER, WRAP_NEVER, DRAW_NORMAL, fgcolor, bgcolor, NULL, NULL);
		x1 = x1 + midl;
	}

	return (y1 + line_height + UI_LINE_WIDTH);
}

//-------------------------------------------------
//  common function for images render
//-------------------------------------------------

std::string ui_menu::arts_render_common(float origx1, float origy1, float origx2, float origy2)
{
	std::string snaptext, searchstr;
	get_title_search(snaptext, searchstr);

	// apply title to right panel
	float title_size = 0.0f;
	float txt_lenght = 0.0f;

	for (int x = FIRST_VIEW; x < LAST_VIEW; x++)
	{
		machine().ui().draw_text_full(container, arts_info[x].title, origx1, origy1, origx2 - origx1, JUSTIFY_CENTER,
		                              WRAP_TRUNCATE, DRAW_NONE, UI_TEXT_COLOR, UI_TEXT_BG_COLOR, &txt_lenght, NULL);
		txt_lenght += 0.01f;
		title_size = MAX(txt_lenght, title_size);
	}

	machine().ui().draw_text_full(container, snaptext.c_str(), origx1, origy1, origx2 - origx1, JUSTIFY_CENTER, WRAP_TRUNCATE,
	                              DRAW_NORMAL, UI_TEXT_COLOR, UI_TEXT_BG_COLOR, NULL, NULL);

	draw_common_arrow(origx1, origy1, origx2, origy2, mewui_globals::curimage_view, FIRST_VIEW, LAST_VIEW, title_size);

	return searchstr;
}

//-------------------------------------------------
//  draw favorites star
//-------------------------------------------------

void ui_menu::draw_star(render_container *container, float x0, float y0)
{
	float y1 = y0 + machine().ui().get_line_height();
	float x1 = x0 + machine().ui().get_line_height() * container->manager().ui_aspect();
	container->add_quad(x0, y0, x1, y1, ARGB_WHITE, star_texture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
}

//-------------------------------------------------
//  draw toolbar
//-------------------------------------------------

void ui_menu::draw_toolbar(render_container *container, float x1, float y1, float x2, float y2, bool software)
{
	// draw a box
	machine().ui().draw_outlined_box(container, x1, y1, x2, y2, rgb_t(0xEF, 0x12, 0x47, 0x7B));

	// take off the borders
	x1 += UI_BOX_LR_BORDER;
	x2 -= UI_BOX_LR_BORDER;
	y1 += UI_BOX_TB_BORDER;
	y2 -= UI_BOX_TB_BORDER;

	render_texture **t_texture = (software) ? sw_toolbar_texture : toolbar_texture;
	bitmap_argb32 **t_bitmap = (software) ? sw_toolbar_bitmap : toolbar_bitmap;

	int m_valid = 0;
	for (int x = 0; x < MEWUI_TOOLBAR_BUTTONS; ++x)
		if (t_bitmap[x]->valid())
			m_valid++;

	float x_pixel = 1.0f / container->manager().ui_target().width();
	x1 = (x1 + x2) * 0.5f - x_pixel * (m_valid * 18);

	for (int z = 0; z < MEWUI_TOOLBAR_BUTTONS; ++z)
	{
		if (t_bitmap[z]->valid())
		{
			x2 = x1 + x_pixel * 32;
			rgb_t color(0xEFEFEFEF);
			if (mouse_hit && x1 <= mouse_x && x2 > mouse_x && y1 <= mouse_y && y2 > mouse_y)
			{
				hover = HOVER_B_FAV + z;
				color = ARGB_WHITE;
				float ypos = y2 + machine().ui().get_line_height() + 2.0f * UI_BOX_TB_BORDER;
				machine().ui().draw_text_box(container, hover_msg[z], JUSTIFY_CENTER, 0.5f, ypos, UI_BACKGROUND_COLOR);
			}

			container->add_quad(x1, y1, x2, y2, color, t_texture[z], PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
			x1 += x_pixel * 36;
		}
	}
}


//-------------------------------------------------
//  perform rendering of image
//-------------------------------------------------

void ui_menu::arts_render_images(bitmap_argb32 *tmp_bitmap, float origx1, float origy1, float origx2, float origy2, bool software)
{
	bool no_available = false;
	float line_height = machine().ui().get_line_height();

	// if it fails, use the default image
	if (!tmp_bitmap->valid())
	{
		tmp_bitmap->reset();
		tmp_bitmap->allocate(256, 256);
		for (int x = 0; x < 256; x++)
			for (int y = 0; y < 256; y++)
				tmp_bitmap->pix32(y, x) = no_avail_bitmap->pix32(y, x);
		no_available = true;
	}

	if (tmp_bitmap->valid())
	{
		float panel_width = origx2 - origx1 - 0.02f;
		float panel_height = origy2 - origy1 - 0.02f - (2.0f * UI_BOX_TB_BORDER) - (2.0f * line_height);
		int screen_width = machine().render().ui_target().width();
		int screen_height = machine().render().ui_target().height();
		int panel_width_pixel = panel_width * screen_width;
		int panel_height_pixel = panel_height * screen_height;
		float ratio = 0.0f;

		// Calculate resize ratios for resizing
		float ratioW = (float)panel_width_pixel / tmp_bitmap->width();
		float ratioH = (float)panel_height_pixel / tmp_bitmap->height();
		float ratioI = (float)tmp_bitmap->height() / tmp_bitmap->width();
		int dest_xPixel = tmp_bitmap->width();
		int dest_yPixel = tmp_bitmap->height();

		// force 4:3 ratio min
		if (machine().options().forced_4x3_snapshot() && ratioI < 0.75f && mewui_globals::curimage_view == SNAPSHOT_VIEW)
		{
			// smaller ratio will ensure that the image fits in the view
			dest_yPixel = tmp_bitmap->width() * 0.75f;
			ratioH = (float)panel_height_pixel / dest_yPixel;
			ratio = MIN(ratioW, ratioH);
			dest_xPixel = tmp_bitmap->width() * ratio;
			dest_yPixel *= ratio;
		}
		// resize the bitmap if necessary
		else if (ratioW < 1 || ratioH < 1 || (machine().options().enlarge_snaps() && !no_available))
		{
			// smaller ratio will ensure that the image fits in the view
			ratio = MIN(ratioW, ratioH);
			dest_xPixel = tmp_bitmap->width() * ratio;
			dest_yPixel = tmp_bitmap->height() * ratio;
		}

		bitmap_argb32 *dest_bitmap;
		dest_bitmap = auto_alloc(machine(), bitmap_argb32);

		// resample if necessary
		if (dest_xPixel != tmp_bitmap->width() || dest_yPixel != tmp_bitmap->height())
		{
			dest_bitmap->allocate(dest_xPixel, dest_yPixel);
			render_color color = { 1.0f, 1.0f, 1.0f, 1.0f };
			render_resample_argb_bitmap_hq(*dest_bitmap, *tmp_bitmap, color, true);
		}
		else
			dest_bitmap = tmp_bitmap;

		snapx_bitmap->reset();
		snapx_bitmap->allocate(panel_width_pixel, panel_height_pixel);
		int x1 = (0.5f * panel_width_pixel) - (0.5f * dest_xPixel);
		int y1 = (0.5f * panel_height_pixel) - (0.5f * dest_yPixel);

		for (int x = 0; x < dest_xPixel; x++)
			for (int y = 0; y < dest_yPixel; y++)
				snapx_bitmap->pix32(y + y1, x + x1) = dest_bitmap->pix32(y, x);

		auto_free(machine(), dest_bitmap);

		// apply bitmap
		snapx_texture->set_bitmap(*snapx_bitmap, snapx_bitmap->cliprect(), TEXFORMAT_ARGB32);
	}
	else
		snapx_bitmap->reset();
}

//-------------------------------------------------
//  draw common arrows
//-------------------------------------------------

void ui_menu::draw_common_arrow(float origx1, float origy1, float origx2, float origy2, int current, int dmin, int dmax, float title_size)
{
	float line_height = machine().ui().get_line_height();
	float lr_arrow_width = 0.4f * line_height * machine().render().ui_aspect();
	float gutter_width = lr_arrow_width * 1.3f;

	// set left-right arrows dimension
	float ar_x0 = 0.5f * (origx2 + origx1) + 0.5f * title_size + gutter_width - lr_arrow_width;
	float ar_y0 = origy1 + 0.1f * line_height;
	float ar_x1 = 0.5f * (origx2 + origx1) + 0.5f * title_size + gutter_width;
	float ar_y1 = origy1 + 0.9f * line_height;

	float al_x0 = 0.5f * (origx2 + origx1) - 0.5f * title_size - gutter_width;
	float al_y0 = origy1 + 0.1f * line_height;
	float al_x1 = 0.5f * (origx2 + origx1) - 0.5f * title_size - gutter_width + lr_arrow_width;
	float al_y1 = origy1 + 0.9f * line_height;

	rgb_t fgcolor_right, fgcolor_left;
	fgcolor_right = fgcolor_left = UI_TEXT_COLOR;

	// set hover
	if (mouse_hit && ar_x0 <= mouse_x && ar_x1 > mouse_x && ar_y0 <= mouse_y && ar_y1 > mouse_y && current!= dmax)
	{
		machine().ui().draw_textured_box(container, ar_x0 + 0.01f, ar_y0, ar_x1 - 0.01f, ar_y1, UI_MOUSEOVER_BG_COLOR, rgb_t(255, 43, 43, 43),
		                                 hilight_main_texture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXWRAP(TRUE));
		hover = HOVER_UI_RIGHT;
		fgcolor_right = UI_MOUSEOVER_COLOR;
	}
	else if (mouse_hit && al_x0 <= mouse_x && al_x1 > mouse_x && al_y0 <= mouse_y && al_y1 > mouse_y && current != dmin)
	{
		machine().ui().draw_textured_box(container, al_x0 + 0.01f, al_y0, al_x1 - 0.01f, al_y1, UI_MOUSEOVER_BG_COLOR, rgb_t(255, 43, 43, 43),
		                                 hilight_main_texture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXWRAP(TRUE));
		hover = HOVER_UI_LEFT;
		fgcolor_left = UI_MOUSEOVER_COLOR;
	}

	// apply arrow
	if (current == dmin)
		container->add_quad(ar_x0, ar_y0, ar_x1, ar_y1, fgcolor_right, arrow_texture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXORIENT(ROT90));
	else if (current == dmax)
		container->add_quad(al_x0, al_y0, al_x1, al_y1, fgcolor_left, arrow_texture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXORIENT(ROT90 ^ ORIENTATION_FLIP_X));
	else
	{
		container->add_quad(ar_x0, ar_y0, ar_x1, ar_y1, fgcolor_right, arrow_texture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXORIENT(ROT90));
		container->add_quad(al_x0, al_y0, al_x1, al_y1, fgcolor_left, arrow_texture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXORIENT(ROT90 ^ ORIENTATION_FLIP_X));
	}
}

//-------------------------------------------------
//  draw icons
//-------------------------------------------------

void ui_menu::draw_icon(render_container *container, int linenum, void *selectedref, float x0, float y0)
{
	static const game_driver *olddriver[MAX_ICONS_RENDER] = { NULL };
	float x1 = x0 + machine().ui().get_line_height() * container->manager().ui_aspect(container);
	float y1 = y0 + machine().ui().get_line_height();
	const game_driver *driver = ((FPTR)selectedref > 2) ? (const game_driver *)selectedref : NULL;

	if (driver == NULL)
		return;

	if (olddriver[linenum] != driver || mewui_globals::redraw_icon)
	{
		olddriver[linenum] = driver;

		// set clone status
		bool cloneof = strcmp(driver->parent, "0");
		if (cloneof)
		{
			int cx = driver_list::find(driver->parent);
			if (cx != -1 && ((driver_list::driver(cx).flags & MACHINE_IS_BIOS_ROOT) != 0))
				cloneof = false;
		}

		// get search path
		path_iterator path(machine().options().icons_directory());
		std::string curpath;
		std::string searchstr(machine().options().icons_directory());

		// iterate over path and add path for zipped formats
		while (path.next(curpath))
			searchstr.append(";").append(curpath.c_str()).append(PATH_SEPARATOR).append("icons");

		emu_file snapfile(searchstr.c_str(), OPEN_FLAG_READ);
		std::string fullname = std::string(driver->name).append(".ico");
		render_load_ico(*icons_bitmap[linenum], snapfile, NULL, fullname.c_str());

		if (!icons_bitmap[linenum]->valid() && cloneof)
		{
			fullname.assign(driver->parent).append(".ico");
			render_load_ico(*icons_bitmap[linenum], snapfile, NULL, fullname.c_str());
		}

		if (icons_bitmap[linenum]->valid())
			icons_texture[linenum]->set_bitmap(*icons_bitmap[linenum], icons_bitmap[linenum]->cliprect(), TEXFORMAT_ARGB32);
	}

	if (icons_bitmap[linenum]->valid())
		container->add_quad(x0, y0, x1, y1, ARGB_WHITE, icons_texture[linenum], PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
}

//-------------------------------------------------
//  draw info arrow
//-------------------------------------------------

void ui_menu::info_arrow(int ub, float origx1, float origx2, float oy1, float line_height, float text_size, float ud_arrow_width)
{
	rgb_t fgcolor = UI_TEXT_COLOR;
	UINT32 orientation = (!ub) ? ROT0 : ROT0 ^ ORIENTATION_FLIP_Y;

	if (mouse_hit && origx1 <= mouse_x && origx2 > mouse_x && oy1 <= mouse_y && oy1 + (line_height * text_size) > mouse_y)
	{
		machine().ui().draw_textured_box(container, origx1 + 0.01f, oy1, origx2 - 0.01f, oy1 + (line_height * text_size), UI_MOUSEOVER_BG_COLOR,
		                                 rgb_t(255, 43, 43, 43), hilight_main_texture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXWRAP(TRUE));
		hover = (!ub) ? HOVER_DAT_UP : HOVER_DAT_DOWN;
		fgcolor = UI_MOUSEOVER_COLOR;
	}

	draw_arrow(container, 0.5f * (origx1 + origx2) - 0.5f * (ud_arrow_width * text_size), oy1 + 0.25f * (line_height * text_size),
	           0.5f * (origx1 + origx2) + 0.5f * (ud_arrow_width * text_size), oy1 + 0.75f * (line_height * text_size), fgcolor, orientation);
}

//-------------------------------------------------
//  draw - draw a menu
//-------------------------------------------------

void ui_menu::draw_palette_menu()
{
	float line_height = machine().ui().get_line_height();
	float lr_arrow_width = 0.4f * line_height * machine().render().ui_aspect();
	float ud_arrow_width = line_height * machine().render().ui_aspect();
	float gutter_width = lr_arrow_width * 1.3f;
	int itemnum, linenum;
	bool mouse_hit, mouse_button;
	float mouse_x = -1, mouse_y = -1;

	if (machine().options().use_background_image() && machine().options().system() == NULL && bgrnd_bitmap->valid())
		container->add_quad(0.0f, 0.0f, 1.0f, 1.0f, ARGB_WHITE, bgrnd_texture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));

	// compute the width and height of the full menu
	float visible_width = 0;
	float visible_main_menu_height = 0;
	for (itemnum = 0; itemnum < item.size(); itemnum++)
	{
		const ui_menu_item &pitem = item[itemnum];

		// compute width of left hand side
		float total_width = gutter_width + machine().ui().get_string_width(pitem.text) + gutter_width;

		// add in width of right hand side
		if (pitem.subtext)
			total_width += 2.0f * gutter_width + machine().ui().get_string_width(pitem.subtext);

		// track the maximum
		if (total_width > visible_width)
			visible_width = total_width;

		// track the height as well
		visible_main_menu_height += line_height;
	}

	// account for extra space at the top and bottom
	float visible_extra_menu_height = customtop + custombottom;

	// add a little bit of slop for rounding
	visible_width += 0.01f;
	visible_main_menu_height += 0.01f;

	// if we are too wide or too tall, clamp it down
	if (visible_width + 2.0f * UI_BOX_LR_BORDER > 1.0f)
		visible_width = 1.0f - 2.0f * UI_BOX_LR_BORDER;

	// if the menu and extra menu won't fit, take away part of the regular menu, it will scroll
	if (visible_main_menu_height + visible_extra_menu_height + 2.0f * UI_BOX_TB_BORDER > 1.0f)
		visible_main_menu_height = 1.0f - 2.0f * UI_BOX_TB_BORDER - visible_extra_menu_height;

	int visible_lines = floor(visible_main_menu_height / line_height);
	visible_main_menu_height = (float)visible_lines * line_height;

	// compute top/left of inner menu area by centering
	float visible_left = (1.0f - visible_width) * 0.5f;
	float visible_top = (1.0f - (visible_main_menu_height + visible_extra_menu_height)) * 0.5f;

	// if the menu is at the bottom of the extra, adjust
	visible_top += customtop;

	// first add us a box
	float x1 = visible_left - UI_BOX_LR_BORDER;
	float y1 = visible_top - UI_BOX_TB_BORDER;
	float x2 = visible_left + visible_width + UI_BOX_LR_BORDER;
	float y2 = visible_top + visible_main_menu_height + UI_BOX_TB_BORDER;
	machine().ui().draw_outlined_box(container, x1, y1, x2, y2, UI_BACKGROUND_COLOR);

	// determine the first visible line based on the current selection
	int top_line = selected - visible_lines / 2;
	if (top_line < 0)
		top_line = 0;
	if (top_line + visible_lines >= item.size())
		top_line = item.size() - visible_lines;

	// determine effective positions taking into account the hilighting arrows
	float effective_width = visible_width - 2.0f * gutter_width;
	float effective_left = visible_left + gutter_width;

	// locate mouse
	mouse_hit = false;
	mouse_button = false;
	INT32 mouse_target_x, mouse_target_y;
	render_target *mouse_target = ui_input_find_mouse(machine(), &mouse_target_x, &mouse_target_y, &mouse_button);
	if (mouse_target != NULL)
		if (mouse_target->map_point_container(mouse_target_x, mouse_target_y, *container, mouse_x, mouse_y))
			mouse_hit = true;

	// loop over visible lines
	hover = item.size() + 1;
	float line_x0 = x1 + 0.5f * UI_LINE_WIDTH;
	float line_x1 = x2 - 0.5f * UI_LINE_WIDTH;

	for (linenum = 0; linenum < visible_lines; linenum++)
	{
		float line_y = visible_top + (float)linenum * line_height;
		itemnum = top_line + linenum;
		const ui_menu_item &pitem = item[itemnum];
		const char *itemtext = pitem.text;
		rgb_t fgcolor = UI_TEXT_COLOR;
		rgb_t bgcolor = UI_TEXT_BG_COLOR;
		rgb_t fgcolor2 = UI_SUBITEM_COLOR;
		rgb_t fgcolor3 = UI_CLONE_COLOR;
		float line_y0 = line_y;
		float line_y1 = line_y + line_height;

		// set the hover if this is our item
		if (mouse_hit && line_x0 <= mouse_x && line_x1 > mouse_x && line_y0 <= mouse_y && line_y1 > mouse_y && pitem.is_selectable())
			hover = itemnum;

		// if we're selected, draw with a different background
		if (itemnum == selected && (pitem.flags & MENU_FLAG_MEWUI_HISTORY) == 0)
		{
			fgcolor = UI_SELECTED_COLOR;
			bgcolor = UI_SELECTED_BG_COLOR;
			fgcolor2 = UI_SELECTED_COLOR;
			fgcolor3 = UI_SELECTED_COLOR;
		}

		// else if the mouse is over this item, draw with a different background
		else if (itemnum == hover && (pitem.flags & MENU_FLAG_MEWUI_HISTORY) == 0)
		{
			fgcolor = UI_MOUSEOVER_COLOR;
			bgcolor = UI_MOUSEOVER_BG_COLOR;
			fgcolor2 = UI_MOUSEOVER_COLOR;
			fgcolor3 = UI_MOUSEOVER_COLOR;
		}

		// if we have some background hilighting to do, add a quad behind everything else
		if (bgcolor != UI_TEXT_BG_COLOR)
			highlight(container, line_x0, line_y0, line_x1, line_y1, bgcolor);

		// if we're on the top line, display the up arrow
		if (linenum == 0 && top_line != 0)
		{
			draw_arrow(container,
			           0.5f * (x1 + x2) - 0.5f * ud_arrow_width,
			           line_y + 0.25f * line_height,
			           0.5f * (x1 + x2) + 0.5f * ud_arrow_width,
			           line_y + 0.75f * line_height,
			           fgcolor,
			           ROT0);
			if (hover == itemnum)
				hover = HOVER_ARROW_UP;
		}

		// if we're on the bottom line, display the down arrow
		else if (linenum == visible_lines - 1 && itemnum != item.size() - 1)
		{
			draw_arrow(container,
			           0.5f * (x1 + x2) - 0.5f * ud_arrow_width,
			           line_y + 0.25f * line_height,
			           0.5f * (x1 + x2) + 0.5f * ud_arrow_width,
			           line_y + 0.75f * line_height,
			           fgcolor,
			           ROT0 ^ ORIENTATION_FLIP_Y);
			if (hover == itemnum)
				hover = HOVER_ARROW_DOWN;
		}

		// if we're just a divider, draw a line
		else if (strcmp(itemtext, MENU_SEPARATOR_ITEM) == 0)
			container->add_line(visible_left, line_y + 0.5f * line_height, visible_left + visible_width, line_y + 0.5f * line_height, UI_LINE_WIDTH, UI_BORDER_COLOR, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));

		// if we don't have a subitem, just draw the string centered
		else if (pitem.subtext == NULL)
			machine().ui().draw_text_full(container, itemtext, effective_left, line_y, effective_width,
			                              JUSTIFY_CENTER, WRAP_TRUNCATE, DRAW_NORMAL, fgcolor, bgcolor, NULL, NULL);

		// otherwise, draw the item on the left and the subitem text on the right
		else
		{
			const char *subitem_text = pitem.subtext;
			rgb_t color = rgb_t((UINT32)strtoul(subitem_text, NULL, 16));

			// draw the left-side text
			machine().ui().draw_text_full(container, itemtext, effective_left, line_y, effective_width,
			                              JUSTIFY_LEFT, WRAP_TRUNCATE, DRAW_NORMAL, fgcolor, bgcolor, NULL, NULL);

			// give 2 spaces worth of padding
			float subitem_width = machine().ui().get_string_width("FF00FF00");

			machine().ui().draw_outlined_box(container, effective_left + effective_width - subitem_width, line_y0,
			                                 effective_left + effective_width, line_y1, color);
		}
	}

	// if there is something special to add, do it by calling the virtual method
	custom_render((selected >= 0 && selected < item.size()) ? item[selected].ref : NULL, customtop, custombottom, x1, y1, x2, y2);

	// return the number of visible lines, minus 1 for top arrow and 1 for bottom arrow
	visitems = visible_lines - (top_line != 0) - (top_line + visible_lines != item.size());
}
