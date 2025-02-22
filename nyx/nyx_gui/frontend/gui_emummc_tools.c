/*
 * Copyright (c) 2019 CTCaer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include "gui.h"
#include "fe_emummc_tools.h"
#include "../config/ini.h"
#include "../libs/fatfs/ff.h"
#include "../storage/sdmmc.h"
#include "../utils/dirlist.h"
#include "../utils/list.h"
#include "../utils/sprintf.h"
#include "../utils/types.h"

extern sdmmc_t sd_sdmmc;
extern sdmmc_storage_t sd_storage;

extern bool sd_mount();
extern void sd_unmount(bool deinit);
extern void emmcsn_path_impl(char *path, char *sub_dir, char *filename, sdmmc_storage_t *storage);

#define MBR_1ST_PART_TYPE_OFF 0x1C2

static int part_idx;
static u32 sector_start;

#pragma GCC push_options
#pragma GCC target ("thumb")

static void _create_window_emummc()
{
	emmc_tool_gui_t emmc_tool_gui_ctxt;

	lv_obj_t *win;
	if (!part_idx)
		win = nyx_create_standard_window(SYMBOL_DRIVE"  Create SD File emuMMC");
	else
		win = nyx_create_standard_window(SYMBOL_DRIVE"  Create SD Partition emuMMC");

	//Disable buttons.
	nyx_window_toggle_buttons(win, true);

	// Chreate important info container.
	lv_obj_t *h1 = lv_cont_create(win, NULL);
	lv_cont_set_fit(h1, false, true);
	lv_obj_set_width(h1, (LV_HOR_RES / 9) * 5);
	lv_obj_set_click(h1, false);
	lv_cont_set_layout(h1, LV_LAYOUT_OFF);

	static lv_style_t h_style;
	lv_style_copy(&h_style, lv_cont_get_style(h1));
	h_style.body.main_color = LV_COLOR_HEX(0x1d1d1d);
	h_style.body.grad_color = h_style.body.main_color;
	h_style.body.opa = LV_OPA_COVER;

	// Chreate log container.
	lv_obj_t *h2 = lv_cont_create(win, h1);
	lv_cont_set_style(h2, &h_style);
	lv_cont_set_fit(h2, false, false);
	lv_obj_set_size(h2, (LV_HOR_RES / 11) * 4, LV_DPI * 5);
	lv_cont_set_layout(h2, LV_LAYOUT_OFF);
	lv_obj_align(h2, h1, LV_ALIGN_OUT_RIGHT_TOP, 0, LV_DPI / 5);

	lv_obj_t *label_log = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_log, true);
	lv_obj_set_style(label_log, &monospace_text);
	lv_label_set_long_mode(label_log, LV_LABEL_LONG_BREAK);
	lv_label_set_static_text(label_log, "");
	lv_obj_set_width(label_log, lv_obj_get_width(h2));
	lv_obj_align(label_log, h2, LV_ALIGN_IN_TOP_LEFT, LV_DPI / 10, LV_DPI / 10);
	emmc_tool_gui_ctxt.label_log = label_log;

	// Create elements for info container.
	lv_obj_t *label_sep = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_info = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_info, true);
	lv_obj_set_width(label_info, lv_obj_get_width(h1));
	lv_label_set_static_text(label_info, "\n\n\n\n\n\n\n\n\n");
	lv_obj_set_style(label_info, lv_theme_get_current()->label.prim);
	lv_obj_align(label_info, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 10);
	emmc_tool_gui_ctxt.label_info = label_info;

	lv_obj_t * bar = lv_bar_create(h1, NULL);
	lv_obj_set_size(bar, LV_DPI * 38 / 10, LV_DPI / 5);
	lv_bar_set_range(bar, 0, 100);
	lv_bar_set_value(bar, 0);
	lv_obj_align(bar, label_info, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 8);
	lv_obj_set_opa_scale(bar, LV_OPA_0);
	lv_obj_set_opa_scale_enable(bar, true);
	emmc_tool_gui_ctxt.bar = bar;

	lv_obj_t *label_pct= lv_label_create(h1, NULL);
	lv_label_set_recolor(label_pct, true);
	lv_label_set_static_text(label_pct, " "SYMBOL_DOT" 0%");
	lv_obj_set_style(label_pct, lv_theme_get_current()->label.prim);
	lv_obj_align(label_pct, bar, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 20, 0);
	lv_obj_set_opa_scale(label_pct, LV_OPA_0);
	lv_obj_set_opa_scale_enable(label_pct, true);
	emmc_tool_gui_ctxt.label_pct = label_pct;

	lv_obj_t *label_finish = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_finish, true);
	lv_label_set_static_text(label_finish, "");
	lv_obj_set_style(label_finish, lv_theme_get_current()->label.prim);
	lv_obj_align(label_finish, bar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI * 9 / 20);
	emmc_tool_gui_ctxt.label_finish = label_finish;
	
	if (!part_idx)
		dump_emummc_file(&emmc_tool_gui_ctxt);
	else
		dump_emummc_raw(&emmc_tool_gui_ctxt, part_idx, sector_start);

	nyx_window_toggle_buttons(win, false);
}


static lv_res_t _create_emummc_raw_action(lv_obj_t * btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);
	lv_obj_t *bg = lv_obj_get_parent(lv_obj_get_parent(btns));

	if (!btn_idx)
	{
		lv_obj_set_style(bg, &lv_style_transp);
		_create_window_emummc();
	}

	part_idx = 0;
	sector_start = 0;

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static void _create_mbox_emummc_raw()
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\222Continue", "\222Cancel", "" };
	static const char *mbox_btn_map2[] = { "\211", "OK", "\211", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	char *txt_buf = (char *)malloc(0x500);
	u8 *mbr = (u8 *)malloc(0x200);

	sd_mount();
	sdmmc_storage_read(&sd_storage, 0, 1, mbr);
	sd_unmount(false);
	memcpy(mbr, mbr + 0x1BE, 0x40);

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4);

	for (int i = 1; i < 4; i++)
	{
		u32 curr_part_size = *(u32 *)&mbr[0x0C + (0x10 * i)];
		sector_start = *(u32 *)&mbr[0x08 + (0x10 * i)];
		u8 type = mbr[0x04 + (0x10 * i)];
		if ((curr_part_size > storage.sec_cnt) && sector_start && type != 0x83) //! TODO: For now it skips linux partitions.
		{
			part_idx = i;
			sector_start += 0x8000;
			break;
		}	
	}

	sdmmc_storage_end(&storage);

	if (part_idx)
	{
		s_printf(txt_buf,
			"#C7EA46 Found applicable partition [%d]!#\n"
			"#FF8000 Do you want to continue?#\n\n", part_idx);
	}
	else
		s_printf(txt_buf, "Failed to find applicable partition!\n\n");

	s_printf(txt_buf + strlen(txt_buf),
		"Partition table:\n"
		"Part 0: Type: %02x, Start: %08x, Size: %08x\n"
		"Part 1: Type: %02x, Start: %08x, Size: %08x\n"
		"Part 2: Type: %02x, Start: %08x, Size: %08x\n"
		"Part 3: Type: %02x, Start: %08x, Size: %08x\n",
		mbr[0x04], *(u32 *)&mbr[0x08], *(u32 *)&mbr[0x0C],
		mbr[0x14], *(u32 *)&mbr[0x18], *(u32 *)&mbr[0x1C],
		mbr[0x24], *(u32 *)&mbr[0x28], *(u32 *)&mbr[0x2C],
		mbr[0x34], *(u32 *)&mbr[0x38], *(u32 *)&mbr[0x3C]);


	lv_mbox_set_text(mbox, txt_buf);
	free(txt_buf);
	free(mbr);

	if (part_idx)
		lv_mbox_add_btns(mbox, mbox_btn_map, _create_emummc_raw_action);
	else
		lv_mbox_add_btns(mbox, mbox_btn_map2, mbox_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);
}

static lv_res_t _create_emummc_action(lv_obj_t * btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);
	lv_obj_t *bg = lv_obj_get_parent(lv_obj_get_parent(btns));

	part_idx = 0;
	sector_start = 0;

	switch (btn_idx)
	{
	case 0:
		lv_obj_set_style(bg, &lv_style_transp);
		_create_window_emummc();
		break;
	case 1:
		_create_mbox_emummc_raw();
		// if available. have max 3 buttons. if selected and used, ask to use the backup tool.
		break;
	}

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static lv_res_t _create_mbox_emummc_create(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\222SD File", "\222SD Partition", "\222Cancel", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	lv_mbox_set_text(mbox,
		"Welcome to #C7EA46 emuMMC# creation tool!\n\n"
		"Please choose what type of emuMMC you want to create.\n"
		"#FF8000 SD File# is saved as files in your FAT partition.\n"
		"#FF8000 SD Partition# is saved as raw image in an available partition.");

	lv_mbox_add_btns(mbox, mbox_btn_map, _create_emummc_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

static void _change_raw_emummc_part_type()
{
	u8 *mbr = (u8 *)malloc(0x200);
	sdmmc_storage_read(&sd_storage, 0, 1, mbr);
	mbr[MBR_1ST_PART_TYPE_OFF + (0x10 * part_idx)] = 0xEE;
	sdmmc_storage_write(&sd_storage, 0, 1, mbr);
	free(mbr);
}

static void _migrate_sd_raw_based()
{
	sector_start += 2;

	sd_mount();
	f_mkdir("emuMMC");
	f_mkdir("emuMMC/ER00");

	f_rename("Emutendo", "emuMMC/ER00/Nintendo");
	FIL fp;
	f_open(&fp, "emuMMC/ER00/raw_based", FA_CREATE_ALWAYS | FA_WRITE);
	f_write(&fp, &sector_start, 4, NULL);
	f_close(&fp);

	_change_raw_emummc_part_type();

	save_emummc_cfg(1, sector_start, "emuMMC/ER00");
	sd_unmount(false);
}

static void _migrate_sd_raw_emummc_based()
{
	char *tmp = (char *)malloc(0x80);
	s_printf(tmp, "emuMMC/RAW%d", part_idx);

	sd_mount();
	f_mkdir("emuMMC");
	f_mkdir(tmp);
	strcat(tmp, "/raw_based");

	FIL fp;
	if (!f_open(&fp, tmp, FA_CREATE_ALWAYS | FA_WRITE))
	{
		f_write(&fp, &sector_start, 4, NULL);
		f_close(&fp);
	}

	s_printf(tmp, "emuMMC/RAW%d", part_idx);

	_change_raw_emummc_part_type();

	save_emummc_cfg(part_idx, sector_start, tmp);

	free(tmp);

	sd_unmount(false);
}

static void _migrate_sd_file_based()
{
	sd_mount();
	f_mkdir("emuMMC");
	f_mkdir("emuMMC/EF00");

	f_rename("Emutendo", "emuMMC/EF00/Nintendo");
	FIL fp;
	f_open(&fp, "emuMMC/EF00/file_based", FA_CREATE_ALWAYS | FA_WRITE);
	f_close(&fp);

	char *path = (char *)malloc(128);
	char *path2 = (char *)malloc(128);
	s_printf(path, "%c%c%c%c%s", 's', 'x', 'o', 's', "/emunand");
	f_rename(path, "emuMMC/EF00/eMMC");

	for (int i = 0; i < 2; i++)
	{
		s_printf(path, "emuMMC/EF00/eMMC/boot%d.bin", i);
		s_printf(path2, "emuMMC/EF00/eMMC/BOOT%d", i);
		f_rename(path, path2);
	}
	for (int i = 0; i < 8; i++)
	{
		s_printf(path, "emuMMC/EF00/eMMC/full.%02d.bin", i);
		s_printf(path2, "emuMMC/EF00/eMMC/%02d", i);
		f_rename(path, path2);
	}

	free(path);
	free(path2);

	save_emummc_cfg(0, 0, "emuMMC/EF00");
	sd_unmount(false);
}

static void _migrate_sd_backup_file_based()
{
	sd_mount();
	f_mkdir("emuMMC");
	f_mkdir("emuMMC/BK00");
	f_mkdir("emuMMC/BK00/eMMC");

	FIL fp;
	f_open(&fp, "emuMMC/BK00/file_based", FA_CREATE_ALWAYS | FA_WRITE);
	f_close(&fp);

	char *path = (char *)malloc(128);
	char *path2 = (char *)malloc(128);
	char *path3 = (char *)malloc(128);

	emmcsn_path_impl(path, "", "", NULL);

	s_printf(path2, "%s/BOOT0", path);
	f_rename(path2, "emuMMC/BK00/eMMC/BOOT0");

	s_printf(path2, "%s/BOOT1", path);
	f_rename(path2, "emuMMC/BK00/eMMC/BOOT1");

	bool multipart = false;
	s_printf(path2, "%s/rawnand.bin", path);

	FILINFO fno;

	if(f_stat(path2, &fno))
		multipart = true;

	if (!multipart)
		f_rename(path2, "emuMMC/BK00/eMMC/00");
	else
	{
		for (int i = 0; i < 32; i++)
		{
			s_printf(path2, "%s/rawnand.bin.%02d", path, i);
			s_printf(path3, "emuMMC/BK00/eMMC/%02d", i);
			if (f_rename(path2, path3))
				break;
		}
	}

	free(path);
	free(path2);
	free(path3);

	save_emummc_cfg(0, 0, "emuMMC/BK00");
	sd_unmount(false);
}

static lv_res_t _create_emummc_mig1_action(lv_obj_t * btns, const char * txt)
{
	switch (lv_btnm_get_pressed(btns))
	{
	case 0:
		_migrate_sd_file_based();
		break;
	case 1:
		_migrate_sd_raw_based();
		break;
	}

	part_idx = 0;
	sector_start = 0;

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static lv_res_t _create_emummc_mig0_action(lv_obj_t * btns, const char * txt)
{
	switch (lv_btnm_get_pressed(btns))
	{
	case 0:
		_migrate_sd_file_based();
		break;
	}

	part_idx = 0;
	sector_start = 0;

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static lv_res_t _create_emummc_mig2_action(lv_obj_t * btns, const char * txt)
{
	switch (lv_btnm_get_pressed(btns))
	{
	case 0:
		_migrate_sd_raw_based();
		break;
	}

	part_idx = 0;
	sector_start = 0;

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static lv_res_t _create_emummc_mig3_action(lv_obj_t * btns, const char * txt)
{
	switch (lv_btnm_get_pressed(btns))
	{
	case 0:
		_migrate_sd_raw_emummc_based();
		break;
	}

	part_idx = 0;
	sector_start = 0;

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static lv_res_t _create_emummc_mig4_action(lv_obj_t * btns, const char * txt)
{
	switch (lv_btnm_get_pressed(btns))
	{
	case 0:
		_migrate_sd_backup_file_based();
		break;
	}

	part_idx = 0;
	sector_start = 0;

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static lv_res_t _create_mbox_emummc_migrate(lv_obj_t *btn)
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\222Continue", "\222Cancel", "" };
	static const char *mbox_btn_map1[] = { "\222SD File", "\222SD Partition", "\222Cancel", "" };
	static const char *mbox_btn_map3[] = { "\211", "OK", "\211", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	char *txt_buf = (char *)malloc(0x500);
	u8 *mbr = (u8 *)malloc(0x200);

	sd_mount();
	sdmmc_storage_read(&sd_storage, 0, 1, mbr);
	
	memcpy(mbr, mbr + 0x1BE, 0x40);

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4);

	bool backup = false;
	bool emummc = false;
	bool file_based = false;
	sector_start = 0;
	part_idx = 0;

	for (int i = 1; i < 4; i++)
	{
		u32 curr_part_size = *(u32 *)&mbr[0x0C + (0x10 * i)];
		sector_start = *(u32 *)&mbr[0x08 + (0x10 * i)];
		if ((curr_part_size > storage.sec_cnt) && sector_start)
		{
			part_idx = i;
			break;
		}	
	}

	//! TODO: What about unallocated

	if (part_idx)
	{
		sdmmc_storage_read(&sd_storage, sector_start + 0xC001, 1, mbr);
		if (!memcmp(mbr, "EFI PART", 8))
		{
			sector_start += 0x8000;
			emummc = true;
		}
		else
		{
			sdmmc_storage_read(&sd_storage, sector_start + 0x4001, 1, mbr);
			if (!memcmp(mbr, "EFI PART", 8))
				emummc = true;
		}

		if (!emummc)
		{
			sdmmc_storage_read(&sd_storage, sector_start + 0x4003, 1, mbr);
			if (memcmp(mbr, "EFI PART", 8))
				part_idx = 0;
		}
	}

	FILINFO fno;
	s_printf(txt_buf, "%c%c%c%c%s", 's', 'x', 'o','s', "/emunand/boot0.bin");

	if(!f_stat(txt_buf, &fno))
		file_based = true;

	bool rawnand_backup_found = false;

	emmcsn_path_impl(txt_buf, "", "BOOT0", &storage);
	if(!f_stat(txt_buf, &fno))
		backup = true;

	emmcsn_path_impl(txt_buf, "", "rawnand.bin", &storage);
	if(!f_stat(txt_buf, &fno))
		rawnand_backup_found = true;

	emmcsn_path_impl(txt_buf, "", "rawnand.bin.00", &storage);
	if(!f_stat(txt_buf, &fno))
		rawnand_backup_found = true;

	if (backup && rawnand_backup_found)
		backup = true;
	else
		backup = false;
	
	sd_unmount(false);
	sdmmc_storage_end(&storage);

	if (backup)
	{
		s_printf(txt_buf,
			"#C7EA46 Found suitable backup for emuMMC!#\n"
			"#FF8000 Do you want to migrate it?#\n\n");
		lv_mbox_add_btns(mbox, mbox_btn_map, _create_emummc_mig4_action);
	}
	else if (emummc)
	{
		s_printf(txt_buf,
			"#C7EA46 Found SD Partition based emuMMC!#\n"
			"#FF8000 Do you want to repair the config for it?#\n\n");
		lv_mbox_add_btns(mbox, mbox_btn_map, _create_emummc_mig3_action);
	}
	else if (part_idx && !file_based)
	{
		s_printf(txt_buf,
			"#C7EA46 Found foreign SD Partition emunand!#\n"
			"#FF8000 Do you want to migrate it?#\n\n");
		lv_mbox_add_btns(mbox, mbox_btn_map, _create_emummc_mig2_action);
	}
	else if (!part_idx && file_based)
	{
		s_printf(txt_buf,
			"#C7EA46 Found foreign SD File emunand!#\n"
			"#FF8000 Do you want to migrate it?#\n\n");
		lv_mbox_add_btns(mbox, mbox_btn_map, _create_emummc_mig0_action);
	}
	else if (part_idx && file_based)
	{
		s_printf(txt_buf,
			"#C7EA46 Found both foreign SD File and Partition emunand!#\n"
			"#FF8000 Choose what to migrate:#\n\n");
		lv_mbox_add_btns(mbox, mbox_btn_map1, _create_emummc_mig1_action);
	}
	else
	{
		s_printf(txt_buf, "No foreign emunand or emuMMC found!\n\n");
		lv_mbox_add_btns(mbox, mbox_btn_map3, mbox_action);
	}

	lv_mbox_set_text(mbox, txt_buf);
	free(txt_buf);
	free(mbr);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

typedef struct _emummc_images_t
{
	char *dirlist;
	u32 part_sector[3];
	u32 part_type[3];
	u32 part_size[3];
	char part_path[3 * 32];
	lv_obj_t *win;
} emummc_images_t;

static emummc_images_t *emummc_img;

static lv_obj_t *emummc_manage_window;

static lv_res_t (*emummc_tools)(lv_obj_t *btn);

static lv_res_t _save_emummc_cfg_mbox_action(lv_obj_t *btns, const char *txt)
{
	free(emummc_img->dirlist);
	lv_obj_del(emummc_img->win);
	lv_obj_del(emummc_manage_window);
	free(emummc_img);
	
	mbox_action(btns, txt);

	(*emummc_tools)(NULL);

	return LV_RES_INV;
}

static void _create_emummc_saved_mbox()
{
	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, &mbox_darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char *mbox_btn_map[] = { "\211", "OK", "\211", "" };
	lv_obj_t * mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 4);

	lv_mbox_set_text(mbox,
		"#FF8000 emuMMC Configuration#\n\n"
		"#96FF00 The emuMMC configuration#\n#96FF00 was saved to sd card!#");

	lv_mbox_add_btns(mbox, mbox_btn_map, _save_emummc_cfg_mbox_action);

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);
}

static lv_res_t _save_raw_emummc_cfg_action(lv_obj_t * btn)
{
	lv_btn_ext_t *ext = lv_obj_get_ext_attr(btn);
	switch (ext->idx)
	{
	case 0:
		save_emummc_cfg(1, emummc_img->part_sector[0], &emummc_img->part_path[0]);
		break;
	case 1:
		save_emummc_cfg(2, emummc_img->part_sector[1], &emummc_img->part_path[32]);
		break;
	case 2:
		save_emummc_cfg(3, emummc_img->part_sector[2], &emummc_img->part_path[64]);
		break;
	}

	_create_emummc_saved_mbox();
	sd_unmount(false);

	return LV_RES_INV;
}

static lv_res_t _save_disable_emummc_cfg_action(lv_obj_t * btn)
{
	save_emummc_cfg(0, 0, NULL);
	_create_emummc_saved_mbox();
	sd_unmount(false);
	

	return LV_RES_INV;
}

static lv_res_t _save_file_emummc_cfg_action(lv_obj_t *btn)
{
	save_emummc_cfg(0, 0, lv_list_get_btn_text(btn));
	_create_emummc_saved_mbox();
	sd_unmount(false);

	return LV_RES_INV;
}

static lv_res_t _create_change_emummc_window()
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_SETTINGS"  Change emuMMC");
	lv_win_add_btn(win, NULL, SYMBOL_POWER"  Disable", _save_disable_emummc_cfg_action);

	sd_mount();

	emummc_img = malloc(sizeof(emummc_images_t));
	emummc_img->win = win;

	u8 *mbr = (u8 *)malloc(0x200);
	char *path = malloc(256);

	sdmmc_storage_read(&sd_storage, 0, 1, mbr);

	memcpy(mbr, mbr + 0x1BE, 0x40);
	memset(emummc_img->part_path, 0, 3 * 32);

	for (int i = 1; i < 4; i++)
	{
		emummc_img->part_size[i - 1] = *(u32 *)&mbr[0x0C + (0x10 * i)];
		emummc_img->part_sector[i - 1] = *(u32 *)&mbr[0x08 + (0x10 * i)];
		emummc_img->part_type[i - 1] = mbr[0x04 + (0x10 * i)];
	}
	free(mbr);

	emummc_img->dirlist = dirlist("emuMMC", NULL, false, true);

	if (!emummc_img->dirlist)
		goto out0;

	u32 emummc_idx = 0;
	FILINFO fno;
	FIL fp;

	// Check for sd raw partitions, based on the folders in /emuMMC.
	while (emummc_img->dirlist[emummc_idx * 256])
	{
		s_printf(path, "emuMMC/%s/raw_based", &emummc_img->dirlist[emummc_idx * 256]);

		if(!f_stat(path, &fno))
		{
			f_open(&fp, path, FA_READ);
			u32 curr_list_sector = 0;
			f_read(&fp, &curr_list_sector, 4, NULL);
			f_close(&fp);

			// Check if there's a HOS image there.
			if (emummc_img->part_sector[0] && curr_list_sector >= emummc_img->part_sector[0] && emummc_img->part_type[0] != 0x83)
			{
				if (emummc_img->part_sector[1] && curr_list_sector >= emummc_img->part_sector[1] && emummc_img->part_type[1] != 0x83)
				{
					if (emummc_img->part_sector[2] && curr_list_sector >= emummc_img->part_sector[2] && emummc_img->part_type[2] != 0x83)
					{
						s_printf(path, "emuMMC/%s", &emummc_img->dirlist[emummc_idx * 256]);
						strcpy(&emummc_img->part_path[2 * 32], path);
						emummc_img->part_sector[2] = curr_list_sector;
					}
					else
					{
						s_printf(path, "emuMMC/%s", &emummc_img->dirlist[emummc_idx * 256]);
						strcpy(&emummc_img->part_path[1 * 32], path);
						emummc_img->part_sector[1] = curr_list_sector;
					}
					
				}
				else
				{
					s_printf(path, "emuMMC/%s", &emummc_img->dirlist[emummc_idx * 256]);
					strcpy(&emummc_img->part_path[0], path);
					emummc_img->part_sector[0] = curr_list_sector;
				}
			}

		}
		emummc_idx++;
	}

	emummc_idx = 0;
	u32 file_based_idx = 0;

	// Sanitize the directory list with sd file based ones.
	while (emummc_img->dirlist[emummc_idx * 256])
	{
		s_printf(path, "emuMMC/%s/file_based", &emummc_img->dirlist[emummc_idx * 256]);

		if(!f_stat(path, &fno))
		{
			strcpy(&emummc_img->dirlist[file_based_idx * 256], &emummc_img->dirlist[emummc_idx * 256]);
			file_based_idx++;
		}
		emummc_idx++;
	}
	emummc_img->dirlist[file_based_idx * 256] = 0;

out0:;
	static lv_style_t h_style;
	lv_style_copy(&h_style, &lv_style_transp);
	h_style.body.padding.inner = 0;
	h_style.body.padding.hor = LV_DPI - (LV_DPI / 4);
	h_style.body.padding.ver = LV_DPI / 6;

	// Create SD Raw Partitions container.
	lv_obj_t *h1 = lv_cont_create(win, NULL);
	lv_cont_set_style(h1, &h_style);
	lv_cont_set_fit(h1, false, true);
	lv_obj_set_width(h1, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(h1, false);
	lv_cont_set_layout(h1, LV_LAYOUT_OFF);

	lv_obj_t *label_sep = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_txt, "SD Raw Partitions");
	lv_obj_set_style(label_txt, lv_theme_get_current()->label.prim);
	lv_obj_align(label_txt, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -(LV_DPI / 2));

	lv_obj_t *line_sep = lv_line_create(h1, NULL);
	static const lv_point_t line_pp[] = { {0, 0}, { LV_HOR_RES - (LV_DPI - (LV_DPI / 4)) * 2, 0} };
	lv_line_set_points(line_sep, line_pp, 2);
	lv_line_set_style(line_sep, lv_theme_get_current()->line.decor);
	lv_obj_align(line_sep, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create RAW 1 button.
	lv_obj_t *btn = lv_btn_create(h1, NULL);
	lv_btn_ext_t *ext = lv_obj_get_ext_attr(btn);
	ext->idx = 0;
	lv_obj_t *btn_label = lv_label_create(btn, NULL);
	if (emummc_img->part_type[0] != 0x83)
		lv_label_set_static_text(btn_label, "SD RAW 1");
	else
		lv_label_set_static_text(btn_label, "Linux");
	if (!emummc_img->part_sector[0] || emummc_img->part_type[0] == 0x83 || !emummc_img->part_path[0])
	{
		lv_btn_set_state(btn, LV_BTN_STATE_INA);
		lv_obj_set_click(btn, false);
	}

	lv_btn_set_fit(btn, false, true);
	lv_obj_set_width(btn, LV_DPI * 3);
	lv_obj_align(btn, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 2, LV_DPI / 5);
	lv_btn_set_action(btn, LV_BTN_ACTION_CLICK, _save_raw_emummc_cfg_action);

	lv_obj_t *lv_desc = lv_label_create(h1, NULL);
	lv_label_set_recolor(lv_desc, true);

	char *txt_buf = malloc(0x500);
	s_printf(txt_buf, "Sector start: 0x%08X\nFolder: %s", emummc_img->part_sector[0], &emummc_img->part_path[0]);
	lv_label_set_array_text(lv_desc, txt_buf, 0x500);
	lv_obj_set_style(lv_desc, &hint_small_style);
	lv_obj_align(lv_desc, btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 5);

	// Create RAW 2 button.
	btn = lv_btn_create(h1, btn);
	ext = lv_obj_get_ext_attr(btn);
	ext->idx = 1;
	btn_label = lv_label_create(btn, btn_label);
	if (emummc_img->part_type[1] != 0x83)
		lv_label_set_static_text(btn_label, "SD RAW 2");
	else
		lv_label_set_static_text(btn_label, "Linux");
	if (!emummc_img->part_sector[1] || emummc_img->part_type[1] == 0x83 || !emummc_img->part_path[32])
	{
		lv_btn_set_state(btn, LV_BTN_STATE_INA);
		lv_obj_set_click(btn, false);
	}
	lv_obj_align(btn, lv_desc, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);
	lv_btn_set_action(btn, LV_BTN_ACTION_CLICK, _save_raw_emummc_cfg_action);

	lv_desc = lv_label_create(h1, lv_desc);
	s_printf(txt_buf, "Sector start: 0x%08X\nFolder: %s", emummc_img->part_sector[1], &emummc_img->part_path[32]);
	lv_label_set_array_text(lv_desc, txt_buf, 0x500);
	lv_obj_align(lv_desc, btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 5);

	// Create RAW 3 button.
	btn = lv_btn_create(h1, btn);
	ext = lv_obj_get_ext_attr(btn);
	ext->idx = 2;
	btn_label = lv_label_create(btn, btn_label);
	if (emummc_img->part_type[2] != 0x83)
		lv_label_set_static_text(btn_label, "SD RAW 3");
	else
		lv_label_set_static_text(btn_label, "Linux");

	if (!emummc_img->part_sector[2] || emummc_img->part_type[2] == 0x83 || !emummc_img->part_path[64])
	{
		lv_btn_set_state(btn, LV_BTN_STATE_INA);
		lv_obj_set_click(btn, false);
	}
	lv_obj_align(btn, lv_desc, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);
	lv_btn_set_action(btn, LV_BTN_ACTION_CLICK, _save_raw_emummc_cfg_action);

	lv_desc = lv_label_create(h1, lv_desc);
	s_printf(txt_buf, "Sector start: 0x%08X\nFolder: %s", emummc_img->part_sector[2], &emummc_img->part_path[64]);
	lv_label_set_array_text(lv_desc, txt_buf, 0x500);
	lv_obj_align(lv_desc, btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 5);

	// Create SD File Based container.
	lv_obj_t *h2 = lv_cont_create(win, NULL);
	lv_cont_set_style(h2, &h_style);
	lv_cont_set_fit(h2, false, true);
	lv_obj_set_width(h2, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(h2, false);
	lv_cont_set_layout(h2, LV_LAYOUT_OFF);
	lv_obj_align(h2, h1, LV_ALIGN_OUT_RIGHT_TOP, LV_DPI * 17 / 29, 0);

	label_sep = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt3 = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_txt3, "SD File Based");
	lv_obj_set_style(label_txt3, lv_theme_get_current()->label.prim);
	lv_obj_align(label_txt3, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI / 7);

	line_sep = lv_line_create(h2, line_sep);
	lv_obj_align(line_sep, label_txt3, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 2), LV_DPI / 8);
	lv_line_set_style(line_sep, lv_theme_get_current()->line.decor);

	lv_obj_t *list_sd_based = lv_list_create(h2, NULL);
	lv_obj_align(list_sd_based, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 2, LV_DPI / 4);

	lv_obj_set_size(list_sd_based, LV_HOR_RES * 4 / 10, LV_VER_RES * 6 / 10);
	lv_list_set_single_mode(list_sd_based, true);

	if (!emummc_img->dirlist)
		goto out1;

	emummc_idx = 0;

	// Add file based to the list.
	while (emummc_img->dirlist[emummc_idx * 256])
	{
		s_printf(path, "emuMMC/%s", &emummc_img->dirlist[emummc_idx * 256]);

		lv_list_add(list_sd_based, NULL, path, _save_file_emummc_cfg_action);

		emummc_idx++;
	}

out1:
	free(path);
	sd_unmount(false);

	return LV_RES_OK;
}

lv_res_t create_win_emummc_tools(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_EDIT"  emuMMC Manage");
	emummc_manage_window = win;

	emummc_tools = (void *)create_win_emummc_tools;

	typedef struct _emummc_cfg_t
	{
		int  enabled;
		u32   sector;
		u16   id;
		char *path;
		char *nintendo_path;
	} emummc_cfg_t;

	emummc_cfg_t emu_info;

	sd_mount();

	emu_info.enabled = 0;
	emu_info.sector = 0;
	emu_info.id = 0;
	emu_info.path = NULL;
	emu_info.nintendo_path = NULL;

	//! TODO: Always update that info when something was changed.
	// Parse emuMMC configuration.
	LIST_INIT(ini_sections);
	if (ini_parse(&ini_sections, "emuMMC/emummc.ini", false))
	{
		LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
		{
			if (!strcmp(ini_sec->name, "emummc"))
			{
				LIST_FOREACH_ENTRY(ini_kv_t, kv, &ini_sec->kvs, link)
				{
					if (!strcmp("enabled", kv->key))
						emu_info.enabled = atoi(kv->val);
					else if (!strcmp("sector", kv->key))
						emu_info.sector = strtol(kv->val, NULL, 16);
					else if (!strcmp("id", kv->key))
						emu_info.id = strtol(kv->val, NULL, 16);
					else if (!strcmp("path", kv->key))
						emu_info.path = kv->val;
					else if (!strcmp("nintendo_path", kv->key))
						emu_info.nintendo_path = kv->val;
				}
			}
		}
	}

	sd_unmount(false);

	static lv_style_t h_style;
	lv_style_copy(&h_style, &lv_style_transp);
	h_style.body.padding.inner = 0;
	h_style.body.padding.hor = LV_DPI - (LV_DPI / 4);
	h_style.body.padding.ver = LV_DPI / 9;

	// Create emuMMC Info & Selection container.
	lv_obj_t *h1 = lv_cont_create(win, NULL);
	lv_cont_set_style(h1, &h_style);
	lv_cont_set_fit(h1, false, true);
	lv_obj_set_width(h1, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(h1, false);
	lv_cont_set_layout(h1, LV_LAYOUT_OFF);

	lv_obj_t *label_sep = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_txt, "emuMMC Info & Selection");
	lv_obj_set_style(label_txt, lv_theme_get_current()->label.prim);
	lv_obj_align(label_txt, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI / 9);

	lv_obj_t *line_sep = lv_line_create(h1, NULL);
	static const lv_point_t line_pp[] = { {0, 0}, { LV_HOR_RES - (LV_DPI - (LV_DPI / 4)) * 2, 0} };
	lv_line_set_points(line_sep, line_pp, 2);
	lv_line_set_style(line_sep, lv_theme_get_current()->line.decor);
	lv_obj_align(line_sep, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create emuMMC info labels.
	lv_obj_t *label_btn = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_btn, true);
	lv_label_set_static_text(label_btn, emu_info.enabled ? "#96FF00 "SYMBOL_OK"  Enabled!#" : "#FF8000 "SYMBOL_CLOSE"  Disabled!#");
	lv_obj_align(label_btn, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);

	lv_obj_t *label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	char *txt_buf = (char *)malloc(0x200);

	if (emu_info.enabled)
	{
		if (emu_info.sector)
			s_printf(txt_buf, "#00DDFF Type:# SD Raw Partition\n#00DDFF Sector:# 0x%08X\n#00DDFF Nintendo folder:# %s",
				emu_info.sector, emu_info.nintendo_path ? emu_info.nintendo_path : "");
		else
			s_printf(txt_buf, "#00DDFF Type:# SD File\n#00DDFF Base folder:# %s\n#00DDFF Nintendo folder:# %s",
				emu_info.path ? emu_info.path : "", emu_info.nintendo_path ? emu_info.nintendo_path : "");

		lv_label_set_array_text(label_txt2, txt_buf, 0x200);
	}
	else
	{
		lv_label_set_static_text(label_txt2, "emuMMC is disabled and eMMC will be used for boot.\n\n");
	}
	
	free(txt_buf);

	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, label_btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create Change emuMMC button.
	lv_obj_t *btn2 = lv_btn_create(h1, NULL);
	lv_btn_set_fit(btn2, true, true);
	label_btn = lv_label_create(btn2, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_SETTINGS"  Change emuMMC");
	lv_obj_align(btn2, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI * 6 / 10);
	lv_btn_set_action(btn2, LV_BTN_ACTION_CLICK, _create_change_emummc_window);

	label_txt2 = lv_label_create(h1, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"Choose between images created in the emuMMC folder\n"
		"or in SD card partitions. You can have at most 3 partition\n"
		"based and countless file based.");

	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	// Create emuMMC Tools container.
	lv_obj_t *h2 = lv_cont_create(win, NULL);
	lv_cont_set_style(h2, &h_style);
	lv_cont_set_fit(h2, false, true);
	lv_obj_set_width(h2, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(h2, false);
	lv_cont_set_layout(h2, LV_LAYOUT_OFF);
	lv_obj_align(h2, h1, LV_ALIGN_OUT_RIGHT_TOP, LV_DPI * 17 / 29, 0);

	label_sep = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt3 = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_txt3, "emuMMC Tools");
	lv_obj_set_style(label_txt3, lv_theme_get_current()->label.prim);
	lv_obj_align(label_txt3, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, 0);

	line_sep = lv_line_create(h2, line_sep);
	lv_obj_align(line_sep, label_txt3, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create Create emuMMC button.
	lv_obj_t *btn3 = lv_btn_create(h2, btn2);
	label_btn = lv_label_create(btn3, NULL);
	lv_btn_set_fit(btn3, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_DRIVE"  Create emuMMC");
	lv_obj_align(btn3, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn3, LV_BTN_ACTION_CLICK, _create_mbox_emummc_create);

	lv_obj_t *label_txt4 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt4, true);
	lv_label_set_static_text(label_txt4,
		"Allows you to create a new #C7EA46 SD File# or #C7EA46 SD Raw Partition#\n"
		"emuMMC. You can create it from eMMC or a eMMC Backup.");
	
	lv_obj_set_style(label_txt4, &hint_small_style);
	lv_obj_align(label_txt4, btn3, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	//! TODO: Move it to a window with multiple choices.
	// Create Migrate emuMMC button.
	lv_obj_t *btn4 = lv_btn_create(h2, btn2);
	label_btn = lv_label_create(btn4, NULL);
	lv_label_set_static_text(label_btn, SYMBOL_SHUFFLE"  Migrate emuMMC");
	lv_obj_align(btn4, label_txt4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 2);
	lv_btn_set_action(btn4, LV_BTN_ACTION_CLICK, NULL);
	lv_btn_set_action(btn4, LV_BTN_ACTION_CLICK, _create_mbox_emummc_migrate);

	label_txt4 = lv_label_create(h2, NULL);
	lv_label_set_recolor(label_txt4, true);
	lv_label_set_static_text(label_txt4,
		"Migrate a backup to a #C7EA46 SD File# or repair existing #C7EA46 SD Raw Partition#.\n"
		"Additionally it allows you to migrate from other emunand\nsolutions.");
		//"Move between #C7EA46 SD File# and #C7EA46 SD Raw Partition# emuMMC.\n"
		//"Additionally it allows you to migrate from other emunand\nsolutions.");
	lv_obj_set_style(label_txt4, &hint_small_style);
	lv_obj_align(label_txt4, btn4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3);

	return LV_RES_OK;
}

#pragma GCC pop_options
