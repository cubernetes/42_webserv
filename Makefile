# Authors: tischmid (timo42@proton.me)
#          sveselov (sveselov@student.42berlin.de)

# config
NAME := webserv
# change to c for C projects and cpp for C++ projects
# source files must still be specified with their extension
EXT := cpp
TEST := c2_unit_tests
UNIT_TEST_DIR := test/unit_tests
CATCH2 := Catch2
TOOLS := script

# tools
# Not using CXX since it defaults to g++, invalidating the logic needed
_CXX ?= c++
#_CXX := clang++ # TODO: @all: make sure it compiles with this
#_CXX := g++     # TODO: @all: make sure it compiles with this
CXX := $(notdir $(shell ls -l $$(which $(_CXX)) | awk '{print $$NF}'))
RM := /bin/rm -f
MKDIR := /bin/mkdir -p

# flags
# CFLAGS := # Don't reset, as some helper scripts in this repo need to adjust CFLAGS
CFLAGS += -O3
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror # TODO: @all: Add back
CFLAGS += -Wshadow
CFLAGS += -Wconversion
CFLAGS += -Wunreachable-code
CFLAGS += -std=c++98
CFLAGS += -MMD
CFLAGS += -MP

CFLAGS += -fdiagnostics-color=always
ifeq ($(strip $(CXX)),clang++)
CFLAGS += -ferror-limit=1
else ifeq ($(strip $(CXX)),g++)
CFLAGS += -fmax-errors=1
endif

CXXFLAGS := -Weffc++

CPPFLAGS := -Isrc -Isrc/HttpServer

# LDFLAGS := # Don't reset, as some helper scripts in this repo need to adjust LDFLAGS
# LDLIBS :=

# additional Catch2 flags
C2_CFLAGS := -std=c++14 -I$(HOME)/.local/include -ICatch2/src -ICatch2/build/generated-includes -Wno-effc++
C2_LDFLAGS := -L$(HOME)/.local/lib -LCatch2/build/src -lCatch2Main -lCatch2

# DEBUG=1 make re # include debugging information in the binary
ifeq ($(DEBUG), 1)
	CFLAGS += -ggdb3 -O0
	LDFLAGS += -ggdb3 -O0
endif

# ASAN=1 make re # check for memory corruptions and some leaks
ifeq ($(ASAN), 1)
	CFLAGS += -fsanitize=address
	LDFLAGS += -fsanitize=address
endif

# TSAN=1 make re # check for thread errors and data races
ifeq ($(TSAN), 1)
	CFLAGS += -fsanitize=thread
	LDFLAGS += -fsanitize=thread
endif

# sources
SRC :=

vpath %.$(EXT) src/HttpServer
SRC += AddingClientSockets.cpp
SRC += EventMonitoring.cpp
SRC += GetRequestHandling.cpp
SRC += HttpServer.cpp
SRC += InitMimeTypes.cpp
SRC += InitStatusTexts.cpp
SRC += LocationMatching.cpp
SRC += RemovingClientSockets.cpp
SRC += RequestHandling.cpp
SRC += ResponseSending.cpp
SRC += Setup.cpp
SRC += SocketManagement.cpp
SRC += SocketUtils.cpp
SRC += TimeoutHandling.cpp 
SRC += UriCanonicalization.cpp

vpath %.$(EXT) src
SRC += Ansi.cpp
SRC += CgiHandler.cpp
SRC += Config.cpp
SRC += Constants.cpp
SRC += DirectiveValidation.cpp
SRC += DirectoryIndexer.cpp
SRC += Errors.cpp
SRC += Logger.cpp
SRC += Options.cpp
SRC += Reflection.cpp
SRC += Repr.cpp
SRC += Utils.cpp
SRC += main.cpp # translation unit with int main(){} MUST be called main.cpp for unit tests to work, see object vars logic below

# additional Catch2 sources
vpath %.$(EXT) $(UNIT_TEST_DIR)
C2_SRC := $(wildcard $(UNIT_TEST_DIR)/*.cpp)
C2_SRC := $(C2_SRC:$(UNIT_TEST_DIR)/%=%) # strip $(UNIT_TEST_DIR) prefix

# object vars
OBJ := $(SRC:.$(EXT)=.o)
OBJ_C2 := $(filter-out main.c2.o, $(SRC:.$(EXT)=.c2.o)) # removes main.cpp
OBJ_C2 += $(C2_SRC:.$(EXT)=.c2.o)                       # adds test unit sources
OBJDIR := obj
OBJDIR_C2 := obj_c2
OBJ := $(addprefix $(OBJDIR)/,$(OBJ))
OBJ_C2 := $(addprefix $(OBJDIR_C2)/,$(OBJ_C2))

# deps (relink also when header files change)
DEPS := $(OBJ:.o=.d)
-include $(DEPS)

DEPS_C2 := $(OBJ_C2:.o=.d)
-include $(DEPS)

# rules
.DEFAULT_GOAL := all

## Build project
all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) $(LDLIBS) -o $@

$(OBJDIR)/%.o: %.$(EXT) | $(OBJDIR)
	$(CXX) $< $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) -c -o $@

$(OBJDIR):
	$(MKDIR) $@

## Build with unit tests and Catch2 main
all_c2: $(CATCH2)
	$(MAKE) $(TEST)

$(TEST): $(OBJ_C2)
	$(CXX) $(OBJ_C2) $(LDFLAGS) $(LDLIBS) $(C2_LDFLAGS) -o $@

$(OBJDIR_C2)/%.c2.o: %.$(EXT) | $(OBJDIR_C2)
	$(_CXX) $< $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) $(C2_CFLAGS) -c -o $@

$(OBJDIR_C2):
	$(MKDIR) $@

$(CATCH2):
	1>/dev/null 2>&1 command -v cmake || { printf '\033[31m%s\033[m\n' 'Cannot build Catch2 without cmake, please install it or remove "$$(CATCH2)" as a dependency of "all_c2" (in the Makefile) if you have Catch2 installed globally (adjust CFLAGS accordingly).'; exit 1; }
	git clone https://github.com/catchorg/Catch2 && \
		cd Catch2 && \
		cmake --install-prefix="$(HOME)/.local" -Bbuild -H. -DBUILD_TESTING=OFF && \
		cmake --build build/ --target Catch2WithMain

# Cleanup
## Remove intermediate files
clean:
	$(RM) $(OBJ)
	$(RM) $(OBJ_C2)
	$(RM) $(DEPS)
	$(RM) $(DEPS_C2)
	$(RM) -r $(OBJDIR)
	$(RM) -r $(OBJDIR_C2)
	$(RM) -r llvm_profdata
	$(RM) -r lcov_report
	$(RM) -r gcovr_report

## Remove intermediate files as well as well as build artefacts
fclean: clean
	$(RM) $(NAME)
	$(RM) $(TEST)

## Rebuild project
re: fclean
	$(MAKE) all

## Build project, then run
run: all
	@printf '\n'
	# This allows $(NAME) to be run using either an absolute, relative or no path.
	# You can pass arguments like this: make run ARGS="hello ' to this world ! ' ."
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) $(ARGS)

## Build project, then run using valgrind memcheck
valrun: all
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && valgrind \
		--leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--track-fds=yes \
		$(NAME) \
		$(ARGS)

# Convenience
## Rebuild project, then run
rerun: re
	$(MAKE) run

## Rebuild project, then run using valgrind memcheck
leakcheck: re
	$(MAKE) valrun

# Tests
## Build with unit tests and Catch2 main, then run
unit_tests: all_c2
	./$(TEST)

## Rebuild with unit tests and Catch2 main, then run
reunit_tests: fclean
	$(MAKE) unit_tests

# Coverage
## Build project, then generate llvmcov coverage report
llvmcov:
	$(TOOLS)/llvmcov.sh

## Rebuild project, then generate llvmcov coverage report
rellvmcov: fclean
	$(MAKE) llvmcov

## Build project, then generate lcov coverage report
lcov:
	$(TOOLS)/lcov.sh

## Rebuild project, then generate lcov coverage report
relcov: fclean
	$(MAKE) lcov

## Build project, then generate gcovr coverage report
gcovr:
	$(TOOLS)/gcovr.sh

## Rebuild project, then generate gcovr coverage report
regcovr: fclean
	$(MAKE) gcovr

### Display this helpful message
h help:
	@printf '\033[31m%b\033[m\n\nTARGETs:\n' "USAGE:\n\tmake <TARGET> [ARGS=\"\"]"
	@<Makefile python3 -c 'exec('"'"'import re\n\nWIDTH = 8\nregex_self_doc = r"## [\\s\\S]*?\\n([a-z_][a-zA-Z -_]*):"\nmatches = list(re.finditer(regex_self_doc, open(0).read()))\nformatted_targets = []\nfor match in matches:\n    target = match.groups()[0]\n    doc_str = "\\n".join(match.group().split("\\n")[:-1]).replace("\\n", " ").replace("## ", "")\n    doc_str_words = doc_str.split()\n    doc_str_words_folded = [doc_str_words[i:i+WIDTH] for i in range(0, len(doc_str_words), WIDTH)]\n    formatted_doc_str = "\\n\\t".join([" ".join(words) for words in doc_str_words_folded])\n    formatted_targets.append(f"\\033[36m{target}\\033[m:\\n\\t{formatted_doc_str}")\nhelp_str = "\\n".join(formatted_targets)\nprint(help_str)\n'"'"')'
	@printf '\n\033[33mNOTES:\n\t%s\033[m\n' 'ARGS only makes sense when the target runs the program'

.PHONY: all
.PHONY: clean fclean
.PHONY: re
.PHONY: run rerun
.PHONY: valrun leakcheck
.PHONY: unit_tests reunit_tests
.PHONY: llvmcov rellvmcov
.PHONY: lcov relcov
.PHONY: gcovr regcovr

# keep intermediate (*.h, *.o, *.d, *.a) targets
.SECONDARY:

# delete failed targets
.DELETE_ON_ERROR:
