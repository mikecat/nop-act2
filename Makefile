COMMON_CFLAGS=-O2 -Wall -Wextra -pedantic -std=c99
COMMON_LDFLAGS=

NATIVE_GCC=gcc
NATIVE_CFLAGS=-DNATIVE_HACK
NATIVE_LD=ld
NATIVE_LDFLAGS=--subsystem 10 -e __start

JOS_GCC=i386-jos-elf-gcc
JOS_CFLAGS=
JOS_LD=i386-jos-elf-ld
JOS_LDFLAGS=-T pe.ld

NATIVE_OBJDIR=objs-native
JOS_OBJDIR=objs-jos

SRCDIR=src

OBJS=start.o serial.o gdt.o interrupts.o interrupts_land.o \
	display.o display_vbe.o font.o terminal.o keyboard.o read_input.o \
	memory.o thread_switch.o

TARGET=bootia32.efi

.PHONY: dummy
dummy:
	@echo "please use \"make native\" or \"make jos\""

.PHONY: native
native: .native-built

.PHONY: jos
jos: .jos-built

.native-built: $(NATIVE_OBJDIR)/bootia32.efi
	cp $^ $(TARGET)
	@touch $@

.jos-built: $(JOS_OBJDIR)/bootia32.efi
	cp $^ $(TARGET)
	@touch $@

$(NATIVE_OBJDIR)/bootia32.efi: $(addprefix $(NATIVE_OBJDIR)/,$(OBJS))
	$(NATIVE_LD) $(COMMON_LDFLAGS) $(NATIVE_LDFLAGS) -o $@ $^

$(JOS_OBJDIR)/bootia32.efi: $(addprefix $(JOS_OBJDIR)/,$(OBJS))
	$(JOS_LD) $(COMMON_LDFLAGS) $(JOS_LDFLAGS) -o $@ $^

$(NATIVE_OBJDIR)/%.o: $(SRCDIR)/%.c .$(NATIVE_OBJDIR)-exists
	$(NATIVE_GCC) $(COMMON_CFLAGS) $(NATIVE_CFLAGS) -c -o $@ $<

$(JOS_OBJDIR)/%.o: $(SRCDIR)/%.c .$(JOS_OBJDIR)-exists
	$(JOS_GCC) $(COMMON_CFLAGS) $(JOS_CFLAGS) -c -o $@ $<

$(NATIVE_OBJDIR)/%.o: $(SRCDIR)/%.s .$(NATIVE_OBJDIR)-exists
	$(NATIVE_GCC) $(COMMON_CFLAGS) $(NATIVE_CFLAGS) -c -o $@ $<

$(JOS_OBJDIR)/%.o: $(SRCDIR)/%.s .$(JOS_OBJDIR)-exists
	$(JOS_GCC) $(COMMON_CFLAGS) $(JOS_CFLAGS) -c -o $@ $<

$(NATIVE_OBJDIR)/%.o: $(SRCDIR)/%.S .$(NATIVE_OBJDIR)-exists
	$(NATIVE_GCC) $(COMMON_CFLAGS) $(NATIVE_CFLAGS) -c -o $@ $<

$(JOS_OBJDIR)/%.o: $(SRCDIR)/%.S .$(JOS_OBJDIR)-exists
	$(JOS_GCC) $(COMMON_CFLAGS) $(JOS_CFLAGS) -c -o $@ $<

$(NATIVE_OBJDIR)/%.o: %.c .$(NATIVE_OBJDIR)-exists
	$(NATIVE_GCC) $(COMMON_CFLAGS) $(NATIVE_CFLAGS) -Isrc -c -o $@ $<

$(JOS_OBJDIR)/%.o: %.c .$(JOS_OBJDIR)-exists
	$(JOS_GCC) $(COMMON_CFLAGS) $(JOS_CFLAGS) -Isrc -c -o $@ $<

.$(NATIVE_OBJDIR)-exists:
	mkdir -p $(NATIVE_OBJDIR)
	@touch $@

.$(JOS_OBJDIR)-exists:
	mkdir -p $(JOS_OBJDIR)
	@touch $@

font.c: font.bmp font_img2src.pl
	perl font_img2src.pl $< $@

.PHONY: clean
clean:
	rm -rf $(NATIVE_OBJDIR) $(JOS_OBJDIR) \
		.$(NATIVE_OBJDIR)-exists .$(JOS_OBJDIR)-exists \
		.native-built .jos-built \
		font.c $(TARGET)
