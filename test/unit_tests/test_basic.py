import pytest
import requests

def test_server_responses(webserver, base_url):
	"""Test basic server responses on different addresses/ports"""
	endpoints = [
		"http://0.0.0.0:8000",
		"http://127.0.0.1:8000",
		"http://localhost:8000",
		"http://127.0.0.1:8000/"
	]
	for endpoint in endpoints:
		response = requests.get(endpoint)
		assert response.status_code == 200

def test_different_methods(webserver, base_url):
	"""Test different HTTP methods"""
	# Test GET
	assert requests.get(base_url).status_code == 200
	
	# Test POST
	assert requests.post(base_url).status_code == 400
	
	# Test DELETE
	assert requests.delete(f"{base_url}/a.txt").status_code == 200
	
	# Test invalid method
	response = requests.request("INVALID", base_url)
	assert response.status_code == 405

@pytest.mark.parametrize("path", [
	"/",
	"/index.html",
	"/index.html/",
	"/nope",
	"/nope/",
	"/.",
	"/..",
	"/...",
	"/../",
	"/../.",
	"/./.",
	"/././",
	"/./index.html",
	"/%69ndex.html",  # URL encoded
	"/%2E%2E%2F"      # URL encoded ../
])
def test_uri_handling(webserver, base_url, path):
	"""Test different URI patterns and edge cases"""
	response = requests.get(f"{base_url}{path}")
	assert response.status_code in [200, 404, 403]
