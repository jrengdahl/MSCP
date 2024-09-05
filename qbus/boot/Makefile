CROSS=pdp11-aout-

ROOT = ..

BINARIES += boot.bin boot.mem

OPT     += -m45
OPT     += -Os
COPT    += -std=gnu17
OPT     += -Wall
OPT     += -Wextra
OPT     += -fno-builtin

OBJS += start.o
OBJS += boot.o
OBJS += subs.o

LDSCRIPT=boot.ld

all: $(BINARIES)

-include $(OBJS:.o=.d)

%.o: %.c
	@echo [CC]  $<
	@$(CROSS)gcc $(OPT) $(COPT) $(OPT.$<) $(INC) $(DEFINES) $(DEFINES.$<) $(WARN) -MMD -c -o $@ $< -Wa,-alh >$*.lst

%.o: %.S
	@echo [AS]  $<
	@$(CROSS)gcc $(OPT) $(OPT.$<) $(INC) $(DEFINES) $(DEFINES.$<) $(WARN) -MMD -c -o $@ $< -Wa,-alh >$*.lst

boot.aout : $(OBJS)
	@echo [LD] $@
	@$(CROSS)gcc -o $@ -T $(LDSCRIPT) -nostdlib $^ -L ../MyLib -lMyLib -lgcc -Wl,-Map=$(basename $@).map
	@$(CROSS)objdump -d $@ >$(basename $@).list

%.bin : %.aout
	@echo [OBJCOPY] $<
	@$(CROSS)objcopy -O binary --only-section=.text --only-section=.data $< $@

%.mem : %.bin
	@echo [BIN2MEM] $<
	@python3 bin2mem.py $< $@

clean:
	@echo [CLEAN]
	@rm -f *.o *.d *.aout *.bin *.map *.list *.lst *.mem $(OBJS)
