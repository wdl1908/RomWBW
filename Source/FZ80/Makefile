DEST=../../Binary

HD1KIMGS = $(DEST)/hd1k_cpm22.img $(DEST)/hd1k_zsdos.img $(DEST)/hd1k_nzcom.img \
	$(DEST)/hd1k_cpm3.img $(DEST)/hd1k_zpm3.img $(DEST)/hd1k_ws4.img

ROMS := $(wildcard $(DEST)/FZ80_*.rom)
ROMS := $(patsubst $(DEST)/%.rom,%,$(ROMS))

OBJECTS := $(patsubst %,%_hd1k_prefix.dat,$(ROMS))
OBJECTS += $(patsubst %,%_hd1k_combo.img,$(ROMS))

TOOLS = ../../Tools

include $(TOOLS)/Makefile.inc

DIFFPATH = $(DIFFTO)/Binary

%_hd1k_prefix.dat: $(DEST)/%.rom
	srec_cat -generate 0x0 0x100000 --constant 0x00 -o temp.dat -binary
	srec_cat temp.dat -binary -exclude 0x1B8 0x200 fz80_ptbl.bin -binary -offset 0x1B8 -o temp.dat -binary
	srec_cat temp.dat -binary -exclude 0x80000 0xE0000 $< -binary -offset 0x80000 -o temp.dat -binary
	mv temp.dat $@

%_hd1k_combo.img: %_hd1k_prefix.dat  $(HD1KIMGS)
	cat $^ > $@
