#! /bin/sh

grep --color=always \
	--exclude-dir=.git/ \
	-inR -- '[T]ODO' | grep -v 'src/\.gitignore'
