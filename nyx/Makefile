ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_rules

################################################################################

NYX_LOAD_ADDR := 0x40003000
NYX_RES_ADDR := 0xEE000000
NYX_STORAGE_ADDR := 0xED000000
NYX_MAGIC := 0x43544347 #"GCTC"
BLVERSION_MAJOR := 0
BLVERSION_MINOR := 8
BLVERSION_HOTFX := 0

BL_RESERVED := 0

################################################################################

TARGET := nyx
BUILDDIR := ./../build
OUTPUTDIR := ./../output
SOURCEDIR = nyx_gui
VPATH = $(dir $(wildcard ./$(SOURCEDIR)/*/)) $(dir $(wildcard ./$(SOURCEDIR)/*/*/))
VPATH += $(dir $(wildcard ./$(SOURCEDIR)/*/*/*/)) $(dir $(wildcard ./$(SOURCEDIR)/*/*/*/*/))

# Main and graphics.
OBJS = $(addprefix $(BUILDDIR)/$(TARGET)/, \
	start.o \
	nyx.o heap.o \
	gfx.o \
	gui.o gui_info.o gui_tools.o gui_emmc_tools.o gui_emummc_tools.o \
	fe_emummc_tools.o fe_emmc_tools.o \
)

# Hardware.
OBJS += $(addprefix $(BUILDDIR)/$(TARGET)/, \
	bpmp.o clock.o cluster.o di.o gpio.o i2c.o mc.o sdram.o pinmux.o se.o smmu.o tsec.o uart.o \
	fuse.o kfuse.o minerva.o \
	sdmmc.o sdmmc_driver.o \
	bq24193.o max17050.o max7762x.o max77620-rtc.o \
	touch.o tmp451.o fan.o \
	hw_init.o \
)

# Utilities.
OBJS += $(addprefix $(BUILDDIR)/$(TARGET)/, \
	btn.o dirlist.o ianos.o util.o \
	config.o ini.o \
	sprintf.o \
)

# Horizon.
OBJS += $(addprefix $(BUILDDIR)/$(TARGET)/, \
	nx_emmc.o \
	hos.o pkg1.o pkg2.o sept.o \
)

# Libraries.
OBJS += $(addprefix $(BUILDDIR)/$(TARGET)/, \
	diskio.o ff.o ffunicode.o ffsystem.o \
	elfload.o elfreloc_arm.o \
	lv_group.o lv_indev.o lv_obj.o lv_refr.o lv_style.o lv_vdb.o \
	lv_draw.o lv_draw_rbasic.o lv_draw_vbasic.o lv_draw_arc.o lv_draw_img.o \
	lv_draw_label.o lv_draw_line.o lv_draw_rect.o lv_draw_triangle.o \
	lv_hal_disp.o lv_hal_indev.o lv_hal_tick.o \
	interui_20.o interui_30.o ubuntu_mono.o hekate_symbol_20.o hekate_symbol_30.o hekate_symbol_120.o lv_font_builtin.o \
	lv_anim.o lv_area.o lv_circ.o lv_color.o lv_font.o lv_ll.o lv_math.o lv_mem.o lv_task.o lv_txt.o lv_gc.o \
	lv_bar.o lv_btn.o lv_btnm.o lv_cb.o lv_cont.o lv_ddlist.o lv_img.o lv_kb.o lv_label.o lv_line.o lv_list.o lv_lmeter.o lv_mbox.o \
	lv_page.o lv_roller.o lv_slider.o lv_sw.o lv_tabview.o lv_ta.o lv_win.o lv_log.o lv_imgbtn.o \
	lv_theme.o lv_theme_hekate.o \
)

################################################################################

CUSTOMDEFINES := -DNYX_RES_ADDR=$(NYX_RES_ADDR) -DNYX_STORAGE_ADDR=$(NYX_STORAGE_ADDR)
CUSTOMDEFINES += -DNYX_LOAD_ADDR=$(NYX_LOAD_ADDR) -DBL_MAGIC=$(NYX_MAGIC)
CUSTOMDEFINES += -DBL_VER_MJ=$(BLVERSION_MAJOR) -DBL_VER_MN=$(BLVERSION_MINOR) -DBL_VER_HF=$(BLVERSION_HOTFX) -DBL_RESERVED=$(BL_RESERVED)

# 0: UART_A, 1: UART_B.
#CUSTOMDEFINES += -DDEBUG_UART_PORT=1

#CUSTOMDEFINES += -DDEBUG

ARCH := -march=armv4t -mtune=arm7tdmi -mthumb-interwork
CFLAGS = $(ARCH) -O2 -nostdlib -ffunction-sections -fdata-sections -flto -fomit-frame-pointer -fno-inline -fno-strict-aliasing -std=gnu11 -Wall $(CUSTOMDEFINES)
CFLAGS += -g
LDFLAGS = $(ARCH) -nostartfiles -lgcc -Wl,--nmagic,--gc-sections -Xlinker --defsym=NYX_LOAD_ADDR=$(NYX_LOAD_ADDR)

################################################################################

.PHONY: all clean

all: $(TARGET).bin
	@echo -n "Nyx size is "
	@wc -c < $(OUTPUTDIR)/$(TARGET).bin

clean:
	@rm -rf $(OBJS)
	@rm -rf $(BUILDDIR)
	@rm -rf $(OUTPUTDIR)

$(TARGET).bin: $(BUILDDIR)/$(TARGET)/$(TARGET).elf
	$(OBJCOPY) -S -O binary $< $(OUTPUTDIR)/$@

$(BUILDDIR)/$(TARGET)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -T $(SOURCEDIR)/link.ld $^ -o $@

$(BUILDDIR)/$(TARGET)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/$(TARGET)/%.o: %.S
	@mkdir -p "$(BUILDDIR)"
	@mkdir -p "$(BUILDDIR)/$(TARGET)"
	@mkdir -p "$(OUTPUTDIR)"
	$(CC) $(CFLAGS) -c $< -o $@
