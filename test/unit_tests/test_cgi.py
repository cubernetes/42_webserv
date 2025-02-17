import pytest
import requests

@pytest.mark.parametrize("method,path,data", [
	("GET", "/test.py", None),
	("POST", "/test.py", {"test": "value"}),
	("GET", "/test.pl", None),
	("POST", "/stdin_test.sh", {"test": "value"})
])
def test_cgi(webserver, base_url_cgi, method, path, data):
	"""Test CGI functionality with different methods and extensions"""
	if method == "GET":
		response = requests.get(f"{base_url_cgi}{path}")
	else:
		response = requests.post(f"{base_url_cgi}{path}", data=data)
	assert response.status_code == 200