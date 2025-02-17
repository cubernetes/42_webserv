import pytest
import requests

def test_autoindex(webserver, base_url):
    """Test directory listing (autoindex)"""
    response = requests.get(f"{base_url}/foo/")
    assert response.status_code == 200
    assert '<html>' in response.text.lower()

def test_file_upload(webserver, base_url, upload_file):
    """Test file upload functionality"""
    with open(upload_file, 'rb') as f:
        files = {'file': (upload_file.name, f)}
        response = requests.post(f"{base_url}/foo", files=files)
    assert response.status_code == 200

@pytest.mark.parametrize("size,expected_status", [
    (1000, 200),    # 1KB - should succeed
    (2000, 413)     # 2KB - should fail
])
def test_client_max_body_size(webserver, base_url, size, expected_status):
    """Test client_max_body_size directive"""
    data = "x" * size
    response = requests.post(f"{base_url}/baz", data=data)
    assert response.status_code == expected_status