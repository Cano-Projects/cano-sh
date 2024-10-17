BUILD_DIR ?= .build

CFLAGS := -std=c99 -pedantic
ifneq ($(EXPLICIT_FLAGS),1)
CFLAGS += @base_flags.txt
else
CFLAGS += $(shell cat base_flags.txt)
endif
CFLAGS += -MMD -MP
CFLAGS += -O2

LDLIBS = -lreadline

VPATH := .
SRC-OUT := main.c
SRC-OUT += repl.c

VPATH += builtins
SRC-OUT += builtin_cd.c
SRC-OUT += builtin_echo.c
SRC-OUT += builtin_exit.c
SRC-OUT += builtin_history.c
SRC-OUT += builtin_kill.c
SRC-OUT += builtin_pwd.c

VPATH += builtins/meta
SRC-OUT += builtins_runner.c


vpath %.c $(VPATH)


# $(eval $(call _mk-recipe-obj, ...))
# args: binary-name, src-list, extra-flags, is_shared
define _mk-bin-recipe

name-$1 := $$(if $$(subst no,,$4),so,out)

ifeq ($4,yes)
so-$1 := $$(BUILD_DIR)/lib$1.so
else
out-$1 := $1
endif

obj-$1 := $$($2:%.c=$$(BUILD_DIR)/$1/%.o)
dep-$1 := $$(obj-$1:%.o=%.d)

-include $$(dep-$1)

$$(BUILD_DIR)/$1/%.o: %.c
	@ mkdir -p $$(dir $$@)
	@ $$(log) "$$(cyan)CC$$(reset) $$(notdir $$@)"
	@ $$(CC)										   \
		$$(CFLAGS) $$(CFLAGS_@$$(notdir $$(@:.o=))) $3 \
		-o $$@ -c $$<

$$($$(name-$1)-$1): $$(obj-$1)
	@ $$(log) "$$(blue)LD$$(reset) $$(notdir $$@)"
	@ $$(CC) -o $$@ $$(obj-$1)                \
		$$(CFLAGS_@$$(notdir $$(@:.o=))) $3 \
		$$(LDLIBS) $$(LDFLAGS)

_clean += $$(obj-$1)
_fclean += $$(out-$1)

endef

# strip the argument spaces, do not append space right after the commas!
_mk-bin = $(call _mk-bin-recipe,$(strip $1),$(strip $2),$3,$(strip $4))

mk-binary = $(call _mk-bin, $1, $2, $3, no)
mk-shared-object = $(call _mk-bin, $1, $2, -shared -fPIC $3, yes)

#? (default): build the release binary
.PHONY: _start
_start: all

#? main: build the release binary, main
$(eval $(call mk-binary, main, SRC-OUT, ))

#? debug: build with debug logs an eponym binary
debug-flags := -fanalyzer -DDEBUG=1 -ggdb2 -g3
$(eval $(call mk-shared-object, debug, SRC-OUT, $(debug-flags)))

debug: hot_reload.c $(so-debug)
	@ $(log) "$(blue)LD$(reset) $@"
	@ $(CC) -o $@ $< $(subst -MMD -MP,,$(CFLAGS))  \
		-DHOT_RELOADER_LIB=$(so-debug)           \
		$(debug-flags) $(LDFLAGS) $(LDLIBS)

_fclean += debug

#? check: build with all warnings and sanitizers an eponym binary
check-flags := $(debug-flags) -fsanitize=address,leak,undefined
$(eval $(call mk-binary, check, SRC-OUT, $(check-flags)))

_fclean += check

.PHONY: all
all: $(out-main)

.PHONY: clean
clean: #? clean: removes object files
	@ $(log) "$(yellow)RM$(reset) OBJS"
	@ $(RM) -r $(_clean)

.PHONY: fclean
fclean: clean #? fclean: remove binaries and object files
	@ $(log) "$(yellow)RM$(reset) $(notdir $(_fclean))"
	@ $(RM) -r $(_fclean)

.PHONY: distclean #? distclean: remove all build artifacts
distclean: fclean
distclean:
	@ $(log) "$(yellow)RM$(reset) $(BUILD_DIR)"
	@ $(RM) -r $(BUILD_DIR)

.PHONY: help
help: #? help: show this help message
	@ grep -P "#[?] " $(MAKEFILE_LIST)          \
      | sed -E 's/.*#\? ([^:]+): (.*)/\1 "\2"/' \
	  | xargs printf "  $(blue)%-12s$(reset): $(cyan)%s$(reset)\n"

.PHONY: bundle #? bundle: build all targets
bundle: all check debug

.PHONY: re
.NOTPARALLEL: re
re: fclean all #? re: rebuild the default target

log = echo -e " " 

ifneq ($(shell command -v tput),)
  ifneq ($(shell tput colors),0)

mk-color = \e[$(strip $1)m

reset := $(call mk-color, 00)

red := $(call mk-color, 31)
green := $(call mk-color, 32)
yellow := $(call mk-color, 33)
blue := $(call mk-color, 34)
purple := $(call mk-color, 35)
cyan := $(call mk-color, 36)

  endif
endif
