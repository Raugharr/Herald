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
#	rm ./src/obj/* 
	$(MAKE) -C $(SRC) clean
