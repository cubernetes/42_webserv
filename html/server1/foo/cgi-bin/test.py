import os
import sys

print("Content-Type: text/html\r\n\r\n")
print("<html><body>")
print("<h1>CGI Test</h1>")
print("<p>Environment variables:</p>")
print("<ul>")
for key, value in os.environ.items():
	print(f"<li>{key}: {value}</li>")
print("</ul>")
print("</body></html>")