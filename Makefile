SRC=src/

.PHONY: src clean

all:
	$(MAKE) -C $(SRC)

clean:
	rm herald*
	$(MAKE) -C $(SRC) clean
