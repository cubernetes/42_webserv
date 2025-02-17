import pytest
import requests

def test_autoindex(webserver, base_url_foo):
    """Test directory listing (autoindex)"""
    response = requests.get(f"{base_url_foo}/foo/")
    assert response.status_code == 200
    assert 'html' in response.text.lower()

def test_file_upload(webserver, base_url_foo, upload_file):
    """Test file upload functionality"""
    with open(upload_file, 'rb') as f:
        files = {'file': (upload_file.name, f)}
        response = requests.post(f"{base_url_foo}/foo", files=files)
    assert response.status_code == 201

@pytest.mark.parametrize("size,expected_status", [
    (5, 201),    # 1KB - should succeed
    (20, 413)     # 2KB - should fail
])
def test_client_max_body_size(webserver, base_url_foo, size, expected_status):
    """Test client_max_body_size directive"""
    data = "x" * size
    response = requests.post(f"{base_url_foo}/baz", data=data)
    assert response.status_code == expected_status