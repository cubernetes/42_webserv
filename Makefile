# Authors: tischmid (timo42@proton.me)
#          sveselov (sveselov@student.42berlin.de)

# config
NAME := webserv
# change to c for C projects and cpp for C++ projects
# source files must still be specified with their extension
EXT := cpp
TEST := c2_tests
TEST_DIR := tests

# tools
CXX ?= c++
#CXX ?= clang++ # TODO: @all: make sure it compiles with this
#CXX ?= g++     # TODO: @all: make sure it compiles with this as well, although not super necessary
RM := /bin/rm -f
MKDIR := /bin/mkdir -p

# flags
# CFLAGS := # Don't reset CFLAGS, as the coverage helper scripts in this repo need to adjust CFLAGS
CFLAGS += -O2
CFLAGS += -Wall
CFLAGS += -Wextra
# CFLAGS += -Werror # TODO: @all: Add back
CFLAGS += -Wshadow
CFLAGS += -Wconversion
CFLAGS += -Wunreachable-code
CFLAGS += -std=c++98
CFLAGS += -MMD
#CFLAGS += -pedantic # Mhhhh maybe don't put this flag, see `-Wno-...' flag below :) # TODO: @all: Remove this and comment to the left
#CFLAGS += -Wno-variadic-macros # C++98 doesn't have variadic macros. Also, emtpy macro argument are UB. But yeahhhh we don't have to be pedantic :))

CFLAGS += -fdiagnostics-color=always
ifeq ($(strip $(CXX)),clang++)
CFLAGS += -ferror-limit=1
else
CFLAGS += -fmax-errors=1
endif

CXXFLAGS :=
CXXFLAGS += -Weffc++

CPPFLAGS := -Isrc

# LDFLAGS := # Don't reset CFLAGS, as the coverage helper scripts in this repo need to adjust CFLAGS

LDLIBS :=

# additional catch2 flags
C2_CFLAGS := -std=c++14
C2_LDFLAGS := -lCatch2Main -lCatch2

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

vpath %.$(EXT) src
SRC += Ansi.cpp
SRC += CgiHandler.cpp
SRC += Config.cpp
SRC += Constants.cpp
SRC += DirectiveValidation.cpp
SRC += Errors.cpp
SRC += HttpServer.cpp
SRC += Logger.cpp
SRC += Reflection.cpp
SRC += Repr.cpp
SRC += Server.cpp
SRC += Utils.cpp
SRC += main.cpp # translation unit with int main(){} MUST be called main.cpp for tests to work, see object vars logic below

# additional catch2 sources
vpath %.$(EXT) $(TEST_DIR)
C2_SRC := $(wildcard $(TEST_DIR)/*.cpp)
C2_SRC := $(C2_SRC:$(TEST_DIR)/%=%) # strip $(TEST_DIR) prefix

# object vars
OBJ := $(SRC:.$(EXT)=.o)
OBJ_C2 := $(filter-out main.c2.o, $(SRC:.$(EXT)=.c2.o)) # removes main.cpp
OBJ_C2 += $(C2_SRC:.$(EXT)=.c2.o)                       # adds test sources
OBJDIR := obj
OBJDIR_C2 := obj_c2
OBJ := $(addprefix $(OBJDIR)/,$(OBJ))
OBJ_C2 := $(addprefix $(OBJDIR_C2)/,$(OBJ_C2))

# deps # relink also when header files change
DEPS := $(OBJ:.o=.d)
-include $(DEPS)

DEPS_C2 := $(OBJ_C2:.o=.d)
-include $(DEPS)

# rules
.DEFAULT_GOAL := all

all: $(NAME)

all_c2: $(TEST)

$(NAME): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) $(LDLIBS) -o $@

$(TEST): $(OBJ_C2)
	$(CXX) $(OBJ_C2) $(LDFLAGS) $(LDLIBS) $(C2_LDFLAGS) -o $@

$(OBJDIR)/%.o: %.$(EXT) | $(OBJDIR)
	$(CXX) $< $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) -c -o $@

$(OBJDIR_C2)/%.c2.o: %.$(EXT) | $(OBJDIR_C2)
	$(CXX) $< $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) $(C2_CFLAGS) -c -o $@

$(OBJDIR):
	$(MKDIR) $@

$(OBJDIR_C2):
	$(MKDIR) $@

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

fclean: clean
	$(RM) $(NAME)
	$(RM) $(TEST)

re: fclean
	make all

# This allows $(NAME) to be run using either an absolute, relative or no path.
# You can pass arguments like this: make run ARGS="hello ' to this world ! ' ."
run: all
	@echo
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) $(ARGS)

valrun: all
	@echo
	@PATH=".$${PATH:+:$${PATH}}" && valgrind \
		--leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--track-fds=yes \
		$(NAME) \
		$(ARGS)

rerun: re
	make run

leakcheck: re
	make valrun

test: all_c2
	./$(TEST)

retest: fclean
	make test

llvmcov:
	./tools/llvmcov.sh

rellvmcov: fclean
	make llvmcov

lcov:
	./tools/lcov.sh

relcov: fclean
	make lcov

gcovr:
	./tools/gcovr.sh

regcovr: fclean
	make gcovr

.PHONY: all clean fclean re run rerun leakcheck test retest llvmcov rellvmcov lcov relcov gcovr regcovr
