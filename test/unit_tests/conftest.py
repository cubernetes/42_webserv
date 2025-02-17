import pytest
import subprocess
import time
import requests
from pathlib import Path

@pytest.fixture(scope="session")
def webserver():
    """Start the webserver for the entire test session"""
    process = subprocess.Popen(["./webserv", "conf/default.conf"])
    time.sleep(2)  # Give server time to start
    yield process
    process.terminate()
    process.wait()

@pytest.fixture(scope="session")
def base_url():
    """Base URL for the default server"""
    return "http://127.0.0.1:8000"

@pytest.fixture(scope="function")
def upload_file(tmp_path):
    """Create a temporary file for upload tests"""
    test_file = tmp_path / "test.txt"
    test_file.write_text("Test content for upload")
    return test_file