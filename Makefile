SRC=src/
TEST=test/

.PHONY: src clean

DEBUG ?=1
ifeq ($(DEBUG), 1)
	export CFLAGS := -DDEBUG -ggdb
else
	export CFLAGS := -DNDEBUG
endif

all:
	$(MAKE) -C $(SRC)
	$(MAKE) -C $(TEST)

test:
	$(MAKE) -C $(TEST)
	./herald-test

clean:
#	rm ./src/obj/* 
	$(MAKE) -C $(SRC) clean
