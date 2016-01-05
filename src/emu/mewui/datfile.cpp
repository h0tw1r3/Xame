// license:BSD-3-Clause
// copyright-holders:Dankan1890
/***************************************************************************

    mewui/datfile.c

    MEWUI DATs manager.

***************************************************************************/

#include "emu.h"
#include "drivenum.h"
#include "mewui/datfile.h"
#include "mewui/utils.h"

//-------------------------------------------------
//  TAGS
//-------------------------------------------------
static std::string DATAFILE_TAG("$");
static std::string TAG_BIO("$bio");
static std::string TAG_INFO("$info");
static std::string TAG_MAME("$mame");
static std::string TAG_COMMAND("$cmd");
static std::string TAG_END("$end");
static std::string TAG_DRIVER("$drv");
static std::string TAG_STORY("$story");
static std::string TAG_HISTORY_R("## REVISION:"); 
static std::string TAG_MAMEINFO_R("# MAMEINFO.DAT");
static std::string TAG_MESSINFO_R("#     MESSINFO.DAT");
static std::string TAG_SYSINFO_R("# This file was generated on");
static std::string TAG_STORY_R("# version");
static std::string TAG_COMMAND_SEPARATOR("-----------------------------------------------");

//-------------------------------------------------
// ctor
//-------------------------------------------------
datfile_manager::datfile_manager(running_machine &machine) : m_machine(machine)
{
	if (machine.options().enabled_dats())
	{
		if (ParseOpen("mameinfo.dat"))
		{
			init_mameinfo();
			ParseClose();
		}

		if (ParseOpen("command.dat"))
		{
			init_command();
			ParseClose();
		}

		if (ParseOpen("story.dat"))
		{
			init_storyinfo();
			ParseClose();
		}

		if (ParseOpen("messinfo.dat"))
		{
			init_messinfo();
			ParseClose();
		}

		if (ParseOpen("sysinfo.dat"))
		{
			init_sysinfo();
			ParseClose();
		}

		if (ParseOpen("history.dat"))
		{
			init_history();
			ParseClose();
		}
	}
}

//-------------------------------------------------
//  initialize sysinfo.dat index
//-------------------------------------------------

void datfile_manager::init_sysinfo()
{
	int swcount = 0;
	int count = index_datafile(m_sysidx, swcount);
	osd_printf_verbose("Sysinfo.dat games found = %i\n", count);
	osd_printf_verbose("Rev = %s\n", m_sysinfo_rev.c_str());
}

//-------------------------------------------------
//  initialize story.dat index
//-------------------------------------------------

void datfile_manager::init_storyinfo()
{
	int swcount = 0;
	int count = index_datafile(m_storyidx, swcount);
	osd_printf_verbose("Story.dat games found = %i\n", count);
}

//-------------------------------------------------
//  initialize history.dat index
//-------------------------------------------------

void datfile_manager::init_history()
{
	int swcount = 0;
	int count = index_datafile(m_histidx, swcount);
	osd_printf_verbose("History.dat games found = %i\n", count);
	osd_printf_verbose("History.dat softwares found = %i\n", swcount);
	osd_printf_verbose("Rev = %s\n", m_history_rev.c_str());
}

//-------------------------------------------------
//  initialize mameinfo.dat index
//-------------------------------------------------

void datfile_manager::init_mameinfo()
{
	int drvcount = 0;
	int count = index_mame_mess_info(m_mameidx, m_drvidx, drvcount);
	osd_printf_verbose("Mameinfo.dat games found = %i\n", count);
	osd_printf_verbose("Mameinfo.dat drivers found = %d\n", drvcount);
	osd_printf_verbose("Rev = %s\n", m_mame_rev.c_str());
}

//-------------------------------------------------
//  initialize messinfo.dat index
//-------------------------------------------------

void datfile_manager::init_messinfo()
{
	int drvcount = 0;
	int count = index_mame_mess_info(m_messidx, m_messdrvidx, drvcount);
	osd_printf_verbose("Messinfo.dat games found = %i\n", count);
	osd_printf_verbose("Messinfo.dat drivers found = %d\n", drvcount);
	osd_printf_verbose("Rev = %s\n", m_mess_rev.c_str());
}

//-------------------------------------------------
//  initialize command.dat index
//-------------------------------------------------

void datfile_manager::init_command()
{
	int swcount = 0;
	int count = index_datafile(m_cmdidx, swcount);
	osd_printf_verbose("Command.dat games found = %i\n", count);
}

//-------------------------------------------------
//  load software info
//-------------------------------------------------

void datfile_manager::load_software_info(std::string &softlist, std::string &buffer, std::string &softname, std::string &parentname)
{
	// Load history text
	if (!m_swindex.empty() && ParseOpen("history.dat"))
	{
		// Find software in software list index
		if (m_swindex.find(softlist) == m_swindex.end())
			return;

		std::vector<Itemsindex>::iterator m_itemsiter;
		m_itemsiter = std::find_if(m_swindex[softlist].begin(), m_swindex[softlist].end(), [softname](Itemsindex const& n) { return n.name == softname; });
		if (m_itemsiter == m_swindex[softlist].end())
			m_itemsiter = std::find_if(m_swindex[softlist].begin(), m_swindex[softlist].end(), [parentname](Itemsindex const& n) { return n.name == parentname; });

		if (m_itemsiter == m_swindex[softlist].end())
			return;

		long s_offset = (*m_itemsiter).offset;
		char rbuf[64*1024];
		fseek(fp, s_offset, SEEK_SET);
//		size_t tend = TAG_END.size();
		std::string readbuf;
		while (fgets(rbuf, 64 * 1024, fp) != nullptr)
		{
			strtrimcarriage(readbuf.assign(rbuf));

			// end entry when a end tag is encountered
			if (readbuf == TAG_END)
				break;

			// add this string to the buffer
			buffer.append(readbuf).append("\n");
		}
		ParseClose();
	}
}

//-------------------------------------------------
//  load_data_info
//-------------------------------------------------

void datfile_manager::load_data_info(const game_driver *drv, std::string &buffer, int type)
{
	std::unordered_map<const game_driver *, long> index_idx;
	std::vector<Itemsindex> driver_idx;
	std::string tag;
	std::string filename;

	switch (type)
	{
		case MEWUI_HISTORY_LOAD:
			filename = "history.dat";
			tag = TAG_BIO;
			index_idx = m_histidx;
			break;
		case MEWUI_MAMEINFO_LOAD:
			filename = "mameinfo.dat";
			tag = TAG_MAME;
			index_idx = m_mameidx;
			driver_idx = m_drvidx;
			break;
		case MEWUI_SYSINFO_LOAD:
			filename = "sysinfo.dat";
			tag = TAG_BIO;
			index_idx = m_sysidx;
			break;
		case MEWUI_MESSINFO_LOAD:
			filename = "messinfo.dat";
			tag = TAG_MAME;
			index_idx = m_messidx;
			driver_idx = m_messdrvidx;
			break;
		case MEWUI_STORY_LOAD:
			filename = "story.dat";
			tag = TAG_STORY;
			index_idx = m_storyidx;
			break;
	}

	if (ParseOpen(filename.c_str()))
	{
		load_data_text(drv, buffer, index_idx, tag);

		// load driver info
		if (!driver_idx.empty())
			load_driver_text(drv, buffer, driver_idx, TAG_DRIVER);

		// cleanup mameinfo double line spacing
		if (tag == TAG_MAME)
			strreplace(buffer, "\n\n", "\n");

		ParseClose();
	}
}

//-------------------------------------------------
//  load a game text into the buffer
//-------------------------------------------------

void datfile_manager::load_data_text(const game_driver *drv, std::string &buffer, DrvIndex &idx, std::string &tag)
{
	std::unordered_map<const game_driver *, long>::iterator m_itemsiter = idx.find(drv);
	if (m_itemsiter == idx.end())
	{
		int cloneof = driver_list::non_bios_clone(*drv);
		if (cloneof == -1)
			return;
		else
		{
			const game_driver *c_drv = &driver_list::driver(cloneof);
			m_itemsiter = idx.find(c_drv);
			if (m_itemsiter == idx.end())
				return;
		}
	}

	long s_offset = (*m_itemsiter).second;
	fseek(fp, s_offset, SEEK_SET);
//	size_t ttag = tag.size();
	char rbuf[64 * 1024];
	std::string readbuf;
	while (fgets(rbuf, 64 * 1024, fp) != nullptr)
	{
		strtrimcarriage(readbuf.assign(rbuf));

		// end entry when a end tag is encountered
		if (readbuf == TAG_END)
			break;

		// continue if a specific tag is encountered
		if (readbuf == tag)
			continue;

		// add this string to the buffer
		buffer.append(readbuf).append("\n");
	}
}

//-------------------------------------------------
//  load a driver name and offset into an
//  indexed array
//-------------------------------------------------

void datfile_manager::load_driver_text(const game_driver *drv, std::string &buffer, std::vector<Itemsindex> &idx, std::string &tag)
{
	std::string s;
	core_filename_extract_base(s, drv->source_file);
	size_t index = 0;
	for (index = 0; index < idx.size() && idx[index].name != s; ++index) ;

	// if driver not found, return
	if (index == idx.size())
		return;

	buffer.append("\n--- DRIVER INFO ---\n").append("Driver: ").append(s).append("\n\n");
	long s_offset = idx[index].offset;
	fseek(fp, s_offset, SEEK_SET);
//	size_t tend = TAG_END.size();
//	size_t ttag = tag.size();
	char rbuf[64 * 1024];
	std::string readbuf;
	while (fgets(rbuf, 64 * 1024, fp) != nullptr)
	{
		strtrimcarriage(readbuf.assign(rbuf));

		// end entry when a end tag is encountered
		if (readbuf == TAG_END)
			break;

		// continue if a specific tag is encountered
		if (readbuf == tag)
			continue;

		// add this string to the buffer
		buffer.append(readbuf).append("\n");
	}
}

//-------------------------------------------------
//  load a game name and offset into an
//  indexed array (mameinfo)
//-------------------------------------------------

int datfile_manager::index_mame_mess_info(DrvIndex &index, std::vector<Itemsindex> &index_drv, int &drvcount)
{
	std::string name;
	size_t t_mame = TAG_MAMEINFO_R.size();
	size_t t_mess = TAG_MESSINFO_R.size();
	size_t t_info = TAG_INFO.size();

	char rbuf[2048];
	std::string readbuf, xid;
	while (fgets(rbuf, 2048, fp) != nullptr)
	{
		strtrimcarriage(readbuf.assign(rbuf));

		if (m_mame_rev.empty() && readbuf.compare(0, t_mame, TAG_MAMEINFO_R) == 0)
		{
			size_t found = readbuf.find(" ", t_mame + 1);
			m_mame_rev = readbuf.substr(t_mame + 1, found - t_mame);
		}
		else if (m_mess_rev.empty() && readbuf.compare(0, t_mess, TAG_MESSINFO_R) == 0)
		{
			size_t found = readbuf.find(" ", t_mess + 1);
			m_mess_rev = readbuf.substr(t_mess + 1, found - t_mess);
		}
	
		// TAG_INFO
		else if (readbuf.compare(0, t_info, TAG_INFO) == 0)
		{
			fgets(rbuf, 2048, fp);
			strtrimcarriage(xid.assign(rbuf));
			name = readbuf.substr(t_info + 1);
			if (xid == TAG_MAME)
			{
				// validate driver
				int game_index = driver_list::find(name.c_str());
				if (game_index != -1)
					index.emplace(&driver_list::driver(game_index), ftell(fp));
			}
			else if (xid == TAG_DRIVER)
			{
				index_drv.emplace_back(name, ftell(fp));
				drvcount++;
			}
		}
	}
	return index.size();
}

//-------------------------------------------------
//  load a game name and offset into an
//  indexed array
//-------------------------------------------------

int datfile_manager::index_datafile(DrvIndex &index, int &swcount)
{
	std::string  readbuf, name;
	size_t t_hist = TAG_HISTORY_R.size();
	size_t t_story = TAG_STORY_R.size();
	size_t t_sysinfo = TAG_SYSINFO_R.size();
	size_t t_info = TAG_INFO.size();
	size_t t_bio = TAG_BIO.size();
	std::string carriage("\r\n");
	char rbuf[2048];
	while (fgets(rbuf, 2048, fp) != nullptr)
	{
		strtrimcarriage(readbuf.assign(rbuf));

		if (m_history_rev.empty() && readbuf.compare(0, t_hist, TAG_HISTORY_R) == 0)
		{
			size_t found = readbuf.find(" ", t_hist + 1);
			m_history_rev = readbuf.substr(t_hist + 1, found - t_hist);
		}
		else if (m_sysinfo_rev.empty() && readbuf.compare(0, t_sysinfo, TAG_SYSINFO_R) == 0)
		{
			size_t found = readbuf.find(".", t_sysinfo + 1);
			m_sysinfo_rev = readbuf.substr(t_sysinfo + 1, found - t_sysinfo);
		}
		else if (m_story_rev.empty() && readbuf.compare(0, t_story, TAG_STORY_R) == 0)
		{
			size_t found = readbuf.find_first_of(carriage, t_story + 1);
			m_story_rev = readbuf.substr(t_story + 1, found - t_story);
		}

		// TAG_INFO identifies the driver
		else if (readbuf.compare(0, t_info, TAG_INFO) == 0)
		{
			int curpoint = t_info + 1;
			int ends = readbuf.length();
			while (curpoint < ends)
			{
				// search for comma
				size_t found = readbuf.find(",", curpoint);

				// found it
				if (found != std::string::npos)
				{
					// copy data and validate driver
					int len = found - curpoint;
					strtrimcarriage(name.assign(readbuf.substr(curpoint, len)));

					// validate driver
					int game_index = driver_list::find(name.c_str());
					if (game_index != -1)
						index.emplace(&driver_list::driver(game_index), ftell(fp));

					// update current point
					curpoint = found + 1;
				}
				// if comma not found, copy data while until reach the end of string
				else if (curpoint < ends)
				{
					strtrimcarriage(name.assign(readbuf.substr(curpoint)));
					int game_index = driver_list::find(name.c_str());
					if (game_index != -1)
							index.emplace(&driver_list::driver(game_index), ftell(fp));

					// update current point
					curpoint = ends;
				}
			}
		}
		// search for software info
		else if (!readbuf.empty() && readbuf[0] == DATAFILE_TAG[0])
		{
			fgets(rbuf, 2048, fp);
			std::string readbuf_2(rbuf);

			// TAG_BIO identifies software list
			if (readbuf_2.compare(0, t_bio, TAG_BIO) == 0)
			{
				size_t eq_sign = readbuf.find("=");
				std::string s_list(readbuf.substr(1, eq_sign - 1));
				std::string s_roms(readbuf.substr(eq_sign + 1));
				int ends = s_list.length();
				int curpoint = 0;

				while (curpoint < ends)
				{
					size_t found = s_list.find(",", curpoint);

					// found it
					if (found != std::string::npos)
					{
						strtrimcarriage(name.assign(s_list.substr(curpoint, found - curpoint)));
						curpoint = found + 1;
					}
					else
					{
						strtrimcarriage(name.assign(s_list));
						curpoint = ends;
					}

					// search for a software list in the index, if not found then allocates
					std::string lname(name);
					int cpoint = 0;
					int cends = s_roms.length();

					while (cpoint < cends)
					{
						// search for comma
						size_t found = s_roms.find(",", cpoint);

						// found it
						if (found != std::string::npos)
						{
							// copy data
							strtrimcarriage(name.assign(s_roms.substr(cpoint, found - cpoint)));

							// add a SoftwareItem
							m_swindex[lname].emplace_back(name, ftell(fp));

							// update current point
							cpoint = found + 1;
							swcount++;
						}
						else
						{
							// if reach the end, bail out
							if (s_roms[cpoint] == '\r' || s_roms[cpoint] == '\n')
								break;

							// copy data
							strtrimcarriage(name.assign(s_roms.substr(cpoint)));

							// add a SoftwareItem
							m_swindex[lname].emplace_back(name, ftell(fp));

							// update current point
							cpoint = cends;
							swcount++;
						}
					}
				}
			}
		}
	}
	return index.size();
}

//---------------------------------------------------------
//	ParseOpen - Open up file for reading
//---------------------------------------------------------

bool datfile_manager::ParseOpen(const char *filename)
{
	// MAME core file parsing functions fail in recognizing UNICODE chars in UTF-8 without BOM,
	// so it's better and faster use standard C fileio functions.

	emu_file file(machine().options().dat_path(), OPEN_FLAG_READ);
	if (file.open(filename) != FILERR_NONE)
		return false;

	m_fullpath = file.fullpath();
	file.close();
	fp = fopen(m_fullpath.c_str(), "r");

	fgetc(fp);
	fseek(fp, 0, SEEK_SET);
	return true;
}

//-------------------------------------------------
//  create the menu index
//-------------------------------------------------

void datfile_manager::index_menuidx(const game_driver *drv, DrvIndex &idx, std::vector<Itemsindex> &index)
{
	std::unordered_map<const game_driver *, long>::iterator m_itemsiter = idx.find(drv);
	if (m_itemsiter == idx.end())
	{
		int cloneof = driver_list::non_bios_clone(*drv);
		if (cloneof == -1)
			return;
		else
		{
			const game_driver *c_drv = &driver_list::driver(cloneof);
			m_itemsiter = idx.find(c_drv);
			if (m_itemsiter == idx.end())
				return;
		}
	}

	// seek to correct point in datafile
	long s_offset = (*m_itemsiter).second;
	fseek(fp, s_offset, SEEK_SET);
	size_t tinfo = TAG_INFO.size();
	char rbuf[64 * 1024];
	std::string readbuf;
	while (fgets(rbuf, 64 * 1024, fp) != nullptr)
	{
		strtrimcarriage(readbuf.assign(rbuf));

		if (!core_strnicmp(TAG_INFO.c_str(), readbuf.c_str(), tinfo))
			break;

		// TAG_COMMAND identifies the driver
		if (readbuf == TAG_COMMAND)
		{
			fgets(rbuf, 64 * 1024, fp);
			strtrimcarriage(readbuf.assign(rbuf));
			index.emplace_back(readbuf, ftell(fp));
		}
	}
}

//-------------------------------------------------
//  load command text into the buffer
//-------------------------------------------------

void datfile_manager::load_command_info(std::string &buffer, const int sel)
{
	if (ParseOpen("command.dat"))
	{
		size_t tcs = TAG_COMMAND_SEPARATOR.size();

		// open and seek to correct point in datafile
		fseek(fp, m_menuidx[sel].offset, SEEK_SET);
		char rbuf[64 * 1024];
		std::string readbuf;
		while (fgets(rbuf, 64 * 1024, fp) != nullptr)
		{
			strtrimcarriage(readbuf.assign(rbuf));

			// skip separator lines
			if (!core_strnicmp(TAG_COMMAND_SEPARATOR.c_str(), readbuf.c_str(), tcs))
				continue;

			// end entry when a tag is encountered
			if (readbuf == TAG_END)
				break;

			// add this string to the buffer
			buffer.append(readbuf).append("\n");;
		}
		ParseClose();
	}
}

//-------------------------------------------------
//  load submenu item for command.dat
//-------------------------------------------------

void datfile_manager::command_sub_menu(const game_driver *drv, std::vector<std::string> &menuitems)
{
	if (ParseOpen("command.dat"))
	{
		m_menuidx.clear();
		index_menuidx(drv, m_cmdidx, m_menuidx);
		for (auto & elem : m_menuidx)
			menuitems.push_back(elem.name);
		ParseClose();
	}
}
