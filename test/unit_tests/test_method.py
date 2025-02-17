import pytest
import requests

@pytest.mark.parametrize("location,allowed_methods", [
    ("/foo", ["GET"]),
    ("/foo/", ["POST"]),
    ("/bar", ["DELETE"]),
    ("/bar/", ["GET", "POST"])
])
def test_method_restrictions(webserver, base_url, location, allowed_methods):
    """Test limit_except directive"""
    all_methods = ["GET", "POST", "DELETE"]
    
    for method in all_methods:
        response = requests.request(method, f"{base_url}{location}")
        if method in allowed_methods:
            assert response.status_code == 200
        else:
            assert response.status_code == 405