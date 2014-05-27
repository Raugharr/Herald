SUBDIR = ./src

all: $(SUBDIR)

$(SUBDIR):
	$(MAKE) -C $@

.PHONY: $(SUBDIR) clean

clean:
	$(MAKE) -C $(SUBDIR) $@
