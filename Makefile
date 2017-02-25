.NOTPARALLEL:

default: submodule_check
	mkdir -p build
	cd build && cmake .. && $(MAKE)

debug: submodule_check
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && $(MAKE)

clean:
	rm -rf build

submodules:
	git submodule update --init --recursive

submodule_check:
	@-test -d .git -a .gitmodules && \
	  git submodule status \
	  | grep -q "^-" \
	  && $(MAKE) submodules || true

.PHONY: default clean submodule_check submodules
