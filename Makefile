SRC=src/

.PHONY: src clean

src:
	$(MAKE) -C $(SRC)

clean:
	rm herald*
	$(MAKE) -C $(SRC) clean
