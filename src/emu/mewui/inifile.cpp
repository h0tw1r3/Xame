// license:BSD-3-Clause
// copyright-holders:Dankan1890
/***************************************************************************

    mewui/inifile.c

    MEWUI INIs file manager.

***************************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "drivenum.h"
#include "mewui/inifile.h"
#include "mewui/utils.h"
#include "softlist.h"

//-------------------------------------------------
//  GLOBAL VARIABLES
//-------------------------------------------------
UINT16 inifile_manager::current_category = 0;
UINT16 inifile_manager::current_file = 0;

//-------------------------------------------------
//  ctor
//-------------------------------------------------

inifile_manager::inifile_manager(running_machine &machine)
	: m_machine(machine)
{
	ini_index.clear();
	directory_scan();
}

//-------------------------------------------------
//  scan directories and create index
//-------------------------------------------------

void inifile_manager::directory_scan()
{
	// open extra INIs folder
	file_enumerator path(machine().options().extraini_path());
	const osd_directory_entry *dir;

	// loop into folder's file
	while ((dir = path.next()) != nullptr)
	{
		int length = strlen(dir->name);
		std::string filename(dir->name);

		// skip mewui_favorite file
		if (!core_stricmp("mewui_favorite.ini", filename.c_str()))
			continue;

		// check .ini file ending
		if ((length > 4) && dir->name[length - 4] == '.' && tolower((UINT8)dir->name[length - 3]) == 'i' &&
			tolower((UINT8)dir->name[length - 2]) == 'n' && tolower((UINT8)dir->name[length - 1]) == 'i')
		{
			// try to open file and indexing
			if (ParseOpen(filename.c_str()))
			{
				init_category(filename);
				ParseClose();
			}
		}
	}
}

//-------------------------------------------------
//  initialize category
//-------------------------------------------------

void inifile_manager::init_category(std::string &filename)
{
	std::vector<IniCategoryIndex> index;
	char rbuf[2048];
	std::string readbuf, name;
	while (fgets(rbuf, 2048, fp) != nullptr)
	{
		readbuf = rbuf;
		if (readbuf[0] == '[')
		{
			size_t found = readbuf.find("]");
			name = readbuf.substr(1, found - 1);
			if (name == "FOLDER_SETTINGS" || name == "ROOT_FOLDER")
				continue;
			else
				index.emplace_back(name, ftell(fp));
		}
	}

	if (!index.empty())
		ini_index.emplace_back(filename, index);
}

//-------------------------------------------------
//  load and indexing ini files
//-------------------------------------------------

void inifile_manager::load_ini_category(std::vector<int> &temp_filter)
{
	if (ini_index.empty())
		return;

	bool search_clones = false;
	std::string filename(ini_index[current_file].name);
	long offset = ini_index[current_file].category[current_category].offset;

	if (!core_stricmp(filename.c_str(), "category.ini") || !core_stricmp(filename.c_str(), "alltime.ini"))
		search_clones = true;

	if (ParseOpen(filename.c_str()))
	{
		fseek(fp, offset, SEEK_SET);
		int num_game = driver_list::total();
		char rbuf[2048];
		std::string readbuf;
		while (fgets(rbuf, 2048, fp) != nullptr)
		{
			strtrimcarriage(readbuf.assign(rbuf));

			if (readbuf.empty() || readbuf[0] == '[')
				break;

			int dfind = driver_list::find(readbuf.c_str());
			if (dfind != -1 && search_clones)
			{
				temp_filter.push_back(dfind);
				int clone_of = driver_list::non_bios_clone(dfind);
				if (clone_of == -1)
				{
					for (int x = 0; x < num_game; x++)
						if (readbuf == driver_list::driver(x).parent && readbuf != driver_list::driver(x).name)
							temp_filter.push_back(x);
				}
			}
			else if (dfind != -1)
				temp_filter.push_back(dfind);
		}
		ParseClose();
	}
}

//---------------------------------------------------------
//	ParseOpen - Open up file for reading
//---------------------------------------------------------

bool inifile_manager::ParseOpen(const char *filename)
{
	// MAME core file parsing functions fail in recognizing UNICODE chars in UTF-8 without BOM,
	// so it's better and faster use standard C fileio functions.

	emu_file file(machine().options().extraini_path(), OPEN_FLAG_READ);
	if (file.open(filename) != FILERR_NONE)
		return false;

	m_fullpath = file.fullpath();
	file.close();
	fp = fopen(m_fullpath.c_str(), "r");

	fgetc(fp);
	fseek(fp, 0, SEEK_SET);
	return true;
}

/**************************************************************************
    FAVORITE MANAGER
**************************************************************************/

//-------------------------------------------------
//  ctor
//-------------------------------------------------

favorite_manager::favorite_manager(running_machine &machine)
	: m_machine(machine)
{
	m_current = -1;
	parse_favorite();
}

//-------------------------------------------------
//  add a game
//-------------------------------------------------

void favorite_manager::add_favorite_game(const game_driver *driver)
{
	m_list.emplace_back(driver->name, driver->description, "", "", "", 0, "", driver, "", "", "", 1, "", "", "", true);
	save_favorite_games();
}

//-------------------------------------------------
//  add a system
//-------------------------------------------------

void favorite_manager::add_favorite_game(ui_software_info &swinfo)
{
	m_list.push_back(swinfo);
	save_favorite_games();
}

//-------------------------------------------------
//  add a game / system
//-------------------------------------------------

void favorite_manager::add_favorite_game()
{
	if ((machine().system().flags & MACHINE_TYPE_ARCADE) != 0)
	{
		add_favorite_game(&machine().system());
		return;
	}

	bool software_avail = false;
	image_interface_iterator iter(machine().root_device());
	for (device_image_interface *image = iter.first(); image != nullptr; image = iter.next())
	{
		if (image->exists() && image->software_entry())
		{
			const software_info *swinfo = image->software_entry();
			const software_part *part = image->part_entry();
			ui_software_info tmpmatches;
			if (swinfo->shortname()) tmpmatches.shortname = swinfo->shortname();
			if (image->longname()) tmpmatches.longname = image->longname();
			if (swinfo->parentname()) tmpmatches.parentname = swinfo->parentname();
			if (image->year()) tmpmatches.year = image->year();
			if (image->manufacturer()) tmpmatches.publisher = image->manufacturer();
			tmpmatches.supported = image->supported();
			if (part->name()) tmpmatches.part = part->name();
			tmpmatches.driver = &machine().system();
			if (image->software_list_name()) tmpmatches.listname = image->software_list_name();
			if (part->interface()) tmpmatches.interface = part->interface();
			if (image->instance_name()) tmpmatches.instance = image->instance_name();
			tmpmatches.startempty = 0;
			tmpmatches.parentlongname.clear();
			if (swinfo->parentname())
			{
				software_list_device *swlist = software_list_device::find_by_name(machine().config(), image->software_list_name());
				for (software_info *c_swinfo = swlist->first_software_info(); c_swinfo != nullptr; c_swinfo = c_swinfo->next())
				{
					std::string c_parent(c_swinfo->parentname());
					if (!c_parent.empty() && c_parent == swinfo->shortname())
						{
							tmpmatches.parentlongname = c_swinfo->longname();
							break;
						}
				}
			}

			tmpmatches.usage.clear();
			for (feature_list_item *flist = swinfo->other_info(); flist != nullptr; flist = flist->next())
				if (!strcmp(flist->name(), "usage"))
					tmpmatches.usage = flist->value();

			if (image->image_type_name()) tmpmatches.devicetype = image->image_type_name();
			tmpmatches.available = true;
			software_avail = true;
			m_list.push_back(tmpmatches);
			save_favorite_games();
		}
	}

	if (!software_avail)
		add_favorite_game(&machine().system());
}

//-------------------------------------------------
//  remove a favorite from list
//-------------------------------------------------

void favorite_manager::remove_favorite_game(ui_software_info &swinfo)
{
	m_list.erase(std::remove(m_list.begin(), m_list.end(), swinfo), m_list.end());
	save_favorite_games();
}

//-------------------------------------------------
//  remove a favorite from list
//-------------------------------------------------

void favorite_manager::remove_favorite_game()
{
	m_list.erase(m_list.begin() + m_current);
	save_favorite_games();
}

//-------------------------------------------------
//  check if game is already in favorite list
//-------------------------------------------------

bool favorite_manager::isgame_favorite()
{
	if ((machine().system().flags & MACHINE_TYPE_ARCADE) != 0)
		return isgame_favorite(&machine().system());

	image_interface_iterator iter(machine().root_device());
	bool image_loaded = false;

	for (device_image_interface *image = iter.first(); image != nullptr; image = iter.next())
	{
		const software_info *swinfo = image->software_entry();
		if (image->exists() && swinfo != nullptr)
		{
			image_loaded = true;
			for (size_t current = 0; current < m_list.size(); current++)
				if (m_list[current].shortname == swinfo->shortname() &&
				    m_list[current].listname == image->software_list_name())
				{
					m_current = current;
					return true;
				}
		}
	}

	if (!image_loaded)
		return isgame_favorite(&machine().system());

	m_current = -1;
	return false;
}

//-------------------------------------------------
//  check if game is already in favorite list
//-------------------------------------------------

bool favorite_manager::isgame_favorite(const game_driver *driver)
{
	for (size_t x = 0; x < m_list.size(); x++)
		if (m_list[x].driver == driver && m_list[x].shortname == driver->name)
		{
			m_current = x;
			return true;
		}

	m_current = -1;
	return false;
}

//-------------------------------------------------
//  check if game is already in favorite list
//-------------------------------------------------

bool favorite_manager::isgame_favorite(ui_software_info &swinfo)
{
	for (size_t x = 0; x < m_list.size(); x++)
		if (m_list[x] == swinfo)
		{
			m_current = x;
			return true;
		}

	m_current = -1;
	return false;
}

//-------------------------------------------------
//  parse favorite file
//-------------------------------------------------

void favorite_manager::parse_favorite()
{
	emu_file file(machine().options().mewui_path(), OPEN_FLAG_READ);
	if (file.open(favorite_filename) == FILERR_NONE)
	{
		char readbuf[1024];
		std::string text;
		file.gets(readbuf, 1024);

		while (readbuf[0] == '[')
			file.gets(readbuf, 1024);

		while (file.gets(readbuf, 1024))
		{
			ui_software_info tmpmatches;
			tmpmatches.shortname = strtrimcarriage(text.assign(readbuf));
			file.gets(readbuf, 1024);
			tmpmatches.longname = strtrimcarriage(text.assign(readbuf));
			file.gets(readbuf, 1024);
			tmpmatches.parentname = strtrimcarriage(text.assign(readbuf));
			file.gets(readbuf, 1024);
			tmpmatches.year = strtrimcarriage(text.assign(readbuf));
			file.gets(readbuf, 1024);
			tmpmatches.publisher = strtrimcarriage(text.assign(readbuf));
			file.gets(readbuf, 1024);
			tmpmatches.supported = atoi(readbuf);
			file.gets(readbuf, 1024);
			tmpmatches.part = strtrimcarriage(text.assign(readbuf));
			file.gets(readbuf, 1024);
			text = strtrimcarriage(text.assign(readbuf));
			int dx = driver_list::find(text.c_str());
			if (dx == -1) continue;
			tmpmatches.driver = &driver_list::driver(dx);
			file.gets(readbuf, 1024);
			tmpmatches.listname = strtrimcarriage(text.assign(readbuf));
			file.gets(readbuf, 1024);
			tmpmatches.interface = strtrimcarriage(text.assign(readbuf));
			file.gets(readbuf, 1024);
			tmpmatches.instance = strtrimcarriage(text.assign(readbuf));
			file.gets(readbuf, 1024);
			tmpmatches.startempty = atoi(readbuf);
			file.gets(readbuf, 1024);
			tmpmatches.parentlongname = strtrimcarriage(text.assign(readbuf));
			file.gets(readbuf, 1024);
			tmpmatches.usage = strtrimcarriage(text.assign(readbuf));
			file.gets(readbuf, 1024);
			tmpmatches.devicetype = strtrimcarriage(text.assign(readbuf));
			file.gets(readbuf, 1024);
			tmpmatches.available = atoi(readbuf);
			m_list.push_back(tmpmatches);
		}
		file.close();
	}
}

//-------------------------------------------------
//  save favorite
//-------------------------------------------------

void favorite_manager::save_favorite_games()
{
	// attempt to open the output file
	emu_file file(machine().options().mewui_path(), OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS);
	if (file.open(favorite_filename) == FILERR_NONE)
	{
		if (m_list.empty())
		{
			file.remove_on_close();
			file.close();
			return;
		}

		// generate the favorite INI
		std::string text("[ROOT_FOLDER]\n[Favorite]\n\n");
		for (auto & elem : m_list)
		{
			text += elem.shortname + "\n";
			text += elem.longname + "\n";
			text += elem.parentname + "\n";
			text += elem.year + "\n";
			text += elem.publisher + "\n";
			strcatprintf(text, "%d\n", elem.supported);
			text += elem.part + "\n";
			strcatprintf(text, "%s\n", elem.driver->name);
			text += elem.listname + "\n";
			text += elem.interface + "\n";
			text += elem.instance + "\n";
			strcatprintf(text, "%d\n", elem.startempty);
			text += elem.parentlongname + "\n";
			text += elem.usage + "\n";
			text += elem.devicetype + "\n";
			strcatprintf(text, "%d\n", elem.available);
		}
		file.puts(text.c_str());
		file.close();
	}
}
