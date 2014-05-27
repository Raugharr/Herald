SUBDIR = ./src

all: $(SUBDIR)

$(SUBDIR):
	$(MAKE) -C $@

.PHONY: $(SUBDIR)
