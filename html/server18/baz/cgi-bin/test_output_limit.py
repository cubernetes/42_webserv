#!/usr/bin/env python3

import sys
import os

# Make stdout unbuffered
os.environ['PYTHONUNBUFFERED'] = '1'

print("Content-Type: text/plain")
print("Transfer-Encoding: chunked")
print("")  # Empty line to separate headers
sys.stdout.flush()

for _ in range(1000000):
	try:
		print("This is a large output line.")
		sys.stdout.flush()
	except BlockingIOError:
		# If we get a blocking error, just continue
		# The CGI handler will read what's available
		continue
