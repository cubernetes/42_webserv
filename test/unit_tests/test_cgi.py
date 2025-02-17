import pytest
import requests

@pytest.mark.parametrize("method,path,data", [
    ("GET", "/foo/test.py", None),
    ("POST", "/foo/test.py", {"test": "value"}),
    ("GET", "/foo/test.pl", None),
    ("POST", "/foo/test.sh", {"test": "value"})
])
def test_cgi(webserver, base_url, method, path, data):
    """Test CGI functionality with different methods and extensions"""
    if method == "GET":
        response = requests.get(f"{base_url}{path}")
    else:
        response = requests.post(f"{base_url}{path}", data=data)
    assert response.status_code == 200