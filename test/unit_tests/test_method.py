import pytest
import requests

@pytest.mark.parametrize("method", [
    ("GET"),
    ("POST"),
    ("DELETE"),
]
)
@pytest.mark.parametrize("location, allowed_methods", [
    ("/foo", ["GET"]),
    ("/foo/", ["POST"]),
    ("/bar", ["DELETE"]),
    ("/bar/", ["GET", "POST"])
])
def test_method_restrictions(webserver, base_url_limit_except, location, allowed_methods, method):
    """Test limit_except directive"""
    
    response = requests.request(method, f"{base_url_limit_except}{location}")
    if method in allowed_methods:
        if method == "POST":
            assert response.status_code == 400
        elif method == "DELETE":
            assert response.status_code == 403
        else:    
            assert response.status_code == 200
    else:
        if method == "POST":
            assert response.status_code == 400
        else:
            assert response.status_code == 405