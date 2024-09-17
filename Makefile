BUILD_DIR ?= .build

CFLAGS := -std=c99 -pedantic
ifneq ($(EXPLICIT_FLAGS),1)
CFLAGS += @base_flags.txt
else
CFLAGS += $(shell cat base_flags.txt)
endif
CFLAGS += -MMD -MP
CFLAGS += -O2

LDLIBS = -lncursesw

VPATH := .
SRC-OUT := main.c
SRC-OUT += repl.c

vpath %.c $(VPATH)

# $(eval $(call mk-recipe-binary, ...))
# args: binary-name, src-list, extra-flags
define _mk-recipe-binary

obj-$1 := $$($2:%.c=$$(BUILD_DIR)/$1/%.o)
out-$1 := $1
dep-$1 := $$(obj-$1:%.o=%.d)

-include $$(dep-$1)

$$(BUILD_DIR)/$1/%.o: %.c
	@ mkdir -p $$(dir $$@)
		$$(CC) \
		$$(CFLAGS) $$(CFLAGS_@$$(notdir $$(@:.o=))) $3 \
		-o $$@ -c $$<

$$(out-$1): $$(obj-$1)
	@ mkdir -p $$(dir $$@)
	$$(CC) -o $$@ $$(obj-$1)                  \
		$$(CFLAGS) $$(CFLAGS_@$$(notdir $$(@:.o=))) $3 \
		$$(LDLIBS) $$(LDFLAGS)

_clean += $$(obj-$1)
_fclean += $$(out-$1)

endef

# strip the argument spaces, do not append space right after the commas!
mk-recipe-binary = $(call _mk-recipe-binary,$(strip $1),$(strip $2),$3)

#? (default): build the release binary
.PHONY: _start
_start: all

#? main: build the release binary, main
$(eval $(call mk-recipe-binary, main, SRC-OUT, ))

#? debug: build with debug logs an eponym binary
debug-flags := -fanalyzer -DDEBUG=1 -ggdb2 -g3
$(eval $(call mk-recipe-binary, debug, SRC-OUT, $(debug-flags)))

#? check: build with all warnings and sanitizers an eponym binary
check-flags := $(debug-flags) -fsanitize=address,leak,undefined
$(eval $(call mk-recipe-binary, check, SRC-OUT, $(check-flags)))

.PHONY: all
all: $(out-main)

.PHONY: clean
clean: #? clean: removes object files
	$(RM) -r $(_clean)

.PHONY: fclean
fclean: clean #? fclean: remove binaries and object files
	$(RM) -r $(_fclean)

.PHONY: distclean #? distclean: remove all build artifacts
distclean: fclean
distclean:
	$(RM) -r $(BUILD_DIR)

.PHONY: help
help: #? help: show this help message
	@ grep -P "#[?] " $(MAKEFILE_LIST)          \
      | sed -E 's/.*#\? ([^:]+): (.*)/\1 "\2"/' \
	  | xargs printf "%-12s: %s\n"

.PHONY: re
.NOTPARALLEL: re
re: fclean all #? re: rebuild the default target
