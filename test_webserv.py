#!/usr/bin/env python3
import unittest
import requests
import subprocess
import time

class WebservTest(unittest.TestCase):
	@classmethod
	def setUpClass(cls):
		# Start the webserver
		cls.webserv_process = subprocess.Popen(["./webserv", "default.conf"])
		# Give it time to start
		time.sleep(2)

	@classmethod
	def tearDownClass(cls):
		# Terminate the webserver
		cls.webserv_process.terminate()
		cls.webserv_process.wait()

	def setUp(self):
		time.sleep(0.1) #between tests

	def test_01_basic_server_responses(self):
		"""Test basic server responses on different addresses/ports"""
		endpoints = [
			"http://0.0.0.0:8000",
			"http://127.0.0.1:8000",
			"http://localhost:8000",
			"http://127.0.0.1:8000/"
		]
		for endpoint in endpoints:
			response = requests.get(endpoint)
			self.assertEqual(response.status_code, 200)

	def test_02_different_methods(self):
		"""Test different HTTP methods"""
		base_url = "http://127.0.0.1:8000"
		
		# Test GET
		response = requests.get(base_url)
		self.assertEqual(response.status_code, 200)
		
		# Test POST
		response = requests.post(base_url)
		self.assertEqual(response.status_code, 200)
		
		# Test DELETE
		response = requests.delete(base_url)
		self.assertEqual(response.status_code, 200)
		
		# Test invalid method
		response = requests.request("INVALID", base_url)
		self.assertEqual(response.status_code, 405)

	def test_03_uri_handling(self):
		"""Test different URI patterns and edge cases"""
		base_url = "http://127.0.0.1:8000"
		test_paths = [
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
			"/%2E%2E%2F",     # URL encoded ../
		]
		
		for path in test_paths:
			response = requests.get(base_url + path)
			self.assertIn(response.status_code, [200, 404, 403])

	def test_04_server_names(self):
		"""Test server_name directive"""
		headers = {'Host': 'server9'}
		response = requests.get("http://127.0.0.1:8000", headers=headers)
		self.assertEqual(response.status_code, 200)

		headers = {'Host': 'myserver'}
		response = requests.get("http://127.0.0.1:8000", headers=headers)
		self.assertEqual(response.status_code, 200)

	def test_05_error_pages(self):
		"""Test custom error pages"""
		base_url = "http://127.0.0.1:8000"
		
		# Test 404
		response = requests.get(f"{base_url}/nonexistent")
		self.assertEqual(response.status_code, 404)
		
		# Test 403
		response = requests.get(f"{base_url}/forbidden")
		self.assertEqual(response.status_code, 403)

	def test_06_location_directives(self):
		"""Test location directives and routing"""
		base_url = "http://127.0.0.1:8000"
		
		# Test basic location
		response = requests.get(f"{base_url}/foo")
		self.assertEqual(response.status_code, 200)
		
		# Test location with trailing slash
		response = requests.get(f"{base_url}/foo/")
		self.assertEqual(response.status_code, 200)

	def test_07_redirects(self):
		"""Test redirect functionality"""
		# Test 301 redirect
		response = requests.get("http://127.0.0.1:8000/bar", allow_redirects=False)
		self.assertEqual(response.status_code, 301)
		self.assertTrue('Location' in response.headers)

	def test_08_autoindex(self):
		"""Test directory listing (autoindex)"""
		response = requests.get("http://127.0.0.1:8000/foo/")
		self.assertEqual(response.status_code, 200)
		self.assertTrue('<html>' in response.text.lower())

	def test_09_file_upload(self):
		"""Test file upload functionality"""
		url = "http://127.0.0.1:8000/foo"
		
		# Create a test file
		test_content = b"Test file content"
		files = {'file': ('test.txt', test_content)}
		
		response = requests.post(url, files=files)
		self.assertEqual(response.status_code, 200)

	def test_10_client_max_body_size(self):
		"""Test client_max_body_size directive"""
		url = "http://127.0.0.1:8000/baz"
		
		# Create a large file that exceeds the limit
		large_content = b"x" * 2000  # 2KB
		files = {'file': ('large.txt', large_content)}
		
		response = requests.post(url, files=files)
		self.assertEqual(response.status_code, 413)  # Payload Too Large

	def test_11_cgi(self):
		"""Test CGI functionality"""
		# Test Python CGI
		response = requests.get("http://127.0.0.1:8000/foo/test.py")
		self.assertEqual(response.status_code, 200)
		
		# Test CGI with POST data
		data = {"test": "value"}
		response = requests.post("http://127.0.0.1:8000/foo/test.py", data=data)
		self.assertEqual(response.status_code, 200)

	def test_12_method_restrictions(self):
		"""Test limit_except directive"""
		url = "http://127.0.0.1:8000/foo"
		
		# Test allowed method
		response = requests.get(url)
		self.assertEqual(response.status_code, 200)
		
		# Test restricted method
		response = requests.post(url)
		self.assertEqual(response.status_code, 405)

if __name__ == '__main__':
	unittest.main(verbosity=2)