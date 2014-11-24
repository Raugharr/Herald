SRC=src/

.PHONY: src clean

DEBUG ?=1
ifeq ($(DEBUG), 1)
	export CFLAGS := -DDEBUG -ggdb
else
	export CFLAGS := -DNDEBUG
endif

all:
	$(MAKE) -C $(SRC)

clean:
	rm herald*
	$(MAKE) -C $(SRC) clean
