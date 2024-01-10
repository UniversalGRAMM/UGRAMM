MAKEFLAGS += -r
DIRECTORIES=$(wildcard */)

# handle the default target case
all: $(patsubst %,MAKE_%,$(DIRECTORIES))
	@# (blank recipe, otherwise the % rule matches)

MAKE_%:
	'$(MAKE)' -C '$*'

### handl all other cases.
%:
	'$(MAKE)' -f '$(word 1, $(MAKEFILE_LIST))' 'ALLDIRS_TARGETED' 'TARGET=$*'

ALLDIRS_TARGETED: $(patsubst %,SINGLE_DIR_TARGETED_%,$(DIRECTORIES))
	@# (blank recipe, otherwise make says 'no recipe')

SINGLE_DIR_TARGETED_%:
	'$(MAKE)' -C '$*' '$(TARGET)'
