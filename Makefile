all:
	$(MAKE) -C simavr all
	$(MAKE) -C libteensylcd all
	$(MAKE) -C teensylcd-run all

clean:
	$(MAKE) -C simavr clean
	$(MAKE) -C libteensylcd clean
	$(MAKE) -C teensylcd-run clean

.PHONY: all clean

