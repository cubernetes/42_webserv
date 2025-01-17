#! /bin/sh

black_fg   () { printf '\x1b\x5b40m'; }
red_fg     () { printf '\x1b\x5b41m'; }
green_fg   () { printf '\x1b\x5b42m'; }
yellow_fg  () { printf '\x1b\x5b43m'; }
blue_fg    () { printf '\x1b\x5b44m'; }
magenta_fg () { printf '\x1b\x5b45m'; }
cyan_fg    () { printf '\x1b\x5b46m'; }
white_fg   () { printf '\x1b\x5b47m'; }
black_bg   () { printf '\x1b\x5b30m'; }
white_bg   () { printf '\x1b\x5b37m'; }
rst        () { printf '\x1b\x5b00m'; }

highlight_tags () {
	sed -e "s/\\(@\\)\\(discuss\\)/$(black_bg)$(yellow_fg)\\1$(rst)$(black_bg)$(yellow_fg)\\2$(rst)/g" |
	sed -e "s/\\(@\\)\\(timo\\)/$(black_bg)$(blue_fg)\\1$(rst)$(black_bg)$(blue_fg)\\2$(rst)/g"        |
	sed -e "s/\\(@\\)\\(sonia\\)/$(black_bg)$(magenta_fg)\\1$(rst)$(black_bg)$(magenta_fg)\\2$(rst)/g" |
	sed -e "s/\\(@\\)\\(all\\)/$(black_bg)$(green_fg)\\1$(rst)$(black_bg)$(green_fg)\\2$(rst)/g"       |
	sed -e "s/\\(@\\)\\(\\w\\+\\)/$(black_bg)$(red_fg)\\1$(rst)$(black_bg)$(red_fg)\\2$(rst)/g"
}

remove_todo_color () {
	sed -e "s/\\[01;31m\\[K[Tt][Oo][Dd][Oo]\\[m\\[K/TODO/g"
}

grep --color=always \
	--exclude-dir=.git/ \
	-inR -- '[T]ODO' "$(git rev-parse --show-toplevel)" | grep -v 'src/\.gitignore' | remove_todo_color | grep -v -e '_TODO_' -e '/TODO/' | highlight_tags
