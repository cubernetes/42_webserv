# Authors: tischmid (timo42@proton.me)
#          sveselov (sveselov@student.42berlin.de)

# config
NAME := webserv
# change to c for C projects and cpp for C++ projects
# source files must still be specified with their extension
EXT := cpp

# tools
CXX := clang++ # TODO change to c++
#CXX := g++ # TODO change to c++
RM := /bin/rm -f
MKDIR := /bin/mkdir -p

# flags
CFLAGS :=
CFLAGS += -O2
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wshadow
#CFLAGS += -pedantic # Mhhhh maybe don't put this flag, see `-Wno-...' flag below :) # TODO: Remove this and comment to the left
#CFLAGS += -Wno-variadic-macros # C++98 doesn't have variadic macros. Also, emtpy macro argument are UB. But yeahhhh we don't have to be pedantic :))
CFLAGS += -Wconversion
CFLAGS += -Wunreachable-code
CFLAGS += -std=c++98
CFLAGS += -MMD
CFLAGS += -fdiagnostics-color=always
# CFLAGS += -Werror

CXXFLAGS :=
CXXFLAGS += -Weffc++

CPPFLAGS :=

LDFLAGS :=

LDLIBS :=

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
SRC += main.cpp
SRC += conf.cpp
SRC += repr.cpp
SRC += ansi.cpp
SRC += Logger.cpp
SRC += Reflection.cpp
SRC += Server.cpp
SRC += HttpServer.cpp
SRC += CgiHandler.cpp
SRC += Constants.cpp
SRC += Errors.cpp
SRC += Utils.cpp

# object vars
OBJ := $(SRC:.$(EXT)=.o)
OBJDIR := obj
OBJ := $(addprefix $(OBJDIR)/,$(OBJ))

# deps # relink also when header files change
DEPS := $(OBJ:.o=.d)
-include $(DEPS)

# rules
.DEFAULT_GOAL := all

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) $(LDLIBS) -o $@

$(OBJDIR)/%.o: %.$(EXT) | $(OBJDIR)
	$(CXX) $< $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) -c -o $@

$(OBJDIR):
	$(MKDIR) $@

clean:
	$(RM) $(OBJ)
	$(RM) $(DEPS)
	$(RM) -r $(OBJDIR)

fclean: clean
	$(RM) $(NAME)

re:
	make fclean
	make all

# This allows $(NAME) to be run using either an absolute, relative or no path.
# You can pass arguments like this: make run ARGS="hello ' to this world ! ' ."
run: 
	make all
	@echo
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) $(ARGS)

valrun:
	make all
	@echo
	@PATH=".$${PATH:+:$${PATH}}" && valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes $(NAME) $(ARGS)

rerun:
	make re
	make run

leakcheck:
	make re
	make valrun

.PHONY: all clean fclean re run rerun leakcheck
