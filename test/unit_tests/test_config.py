
import pytest
import requests

@pytest.mark.parametrize("server_name", ["server9", "myserver"])
def test_server_names(webserver, base_url, server_name):
    """Test server_name directive"""
    headers = {'Host': server_name}
    response = requests.get(base_url, headers=headers)
    assert response.status_code == 200

@pytest.mark.parametrize("error_code,path", [
    (404, "/nonexistent"),
    (403, "/forbidden")
])
def test_error_pages(webserver, base_url, error_code, path):
    """Test custom error pages"""
    response = requests.get(f"{base_url}{path}")
    assert response.status_code == error_code

def test_location_directives(webserver, base_url):
    """Test location directives and routing"""
    # Test basic location
    assert requests.get(f"{base_url}/foo").status_code == 200
    
    # Test location with trailing slash
    assert requests.get(f"{base_url}/foo/").status_code == 200

def test_redirects(webserver, base_url):
    """Test redirect functionality"""
    response = requests.get(f"{base_url}/bar", allow_redirects=False)
    assert response.status_code == 301
    assert 'Location' in response.headers