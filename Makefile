.NOTPARALLEL:

all: submodule_check
	mkdir -p build
	cd build && cmake .. && $(MAKE)


clean:
	rm -rf build

submodules:
	git submodule update --init --recursive

submodule_check:
	@-test -d .git -a .gitmodules && \
	  git submodule status \
	  | grep -q "^-" \
	  && $(MAKE) submodules || true

.PHONY: all clean submodule_check submodules
