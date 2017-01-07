SRC=src/
TEST=test/

.PHONY: src clean

DEBUG ?=1
ifeq ($(DEBUG), 1)
	export CFLAGS := -Wall -std=c99 -DDEBUG -ggdb3 -pg
	export OBJFLD := objdebug/
else
	export CFLAGS := -Wall -std=c99 -DNDEBUG -O3
	export OBJFLD := objrelease/
endif
export CC := gcc 

all:
	$(MAKE) -C $(SRC)
	$(MAKE) -C $(TEST)

test:
	$(MAKE) -C $(TEST)
	./herald-test

clean:
#	rm ./src/obj/* 
	$(MAKE) -C $(SRC) clean
