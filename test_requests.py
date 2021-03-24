import pytest
import requests
import requests_curl


def test_bad_http_version_value():
    with pytest.raises(ValueError):
        requests_curl.CurlEasyAdapter(http_version='abc')


def test_bad_http_version_type():
    with pytest.raises(TypeError):
        requests_curl.CurlEasyAdapter(http_version=1.1)


def test_google():
    import datetime

    # TODO: implement requests_curl.get
    response = requests_curl.request('get', 'https://www.google.com/', http_version='1.1')

    assert response.apparent_encoding == 'ISO-8859-1'
    # assert response.cookies == None  # TODO
    assert isinstance(response.elapsed, datetime.timedelta)
    assert response.encoding == None  # TODO
    assert 'Date' in response.headers
    assert response.headers['date'] == response.headers['Date']
    assert response.headers['Expires'] == '-1'
    # assert response.history == None  # TODO
    assert response.is_permanent_redirect == False
    assert response.is_redirect == False
    # assert response.links == None  # TODO
    assert response.next == None  # TODO
    assert response.ok == True
    assert response.raw == None  # TODO
    assert response.reason == 'OK'
    assert isinstance(response.request, requests.PreparedRequest)
    assert response.status_code == 200
    assert response.text == response.content.decode('latin-1')
    assert response.url == 'https://www.google.com/'


def test_get():
    session = requests_curl.CurlEasySession()

    response = session.get('https://httpbin.org/get', params={'val1': 4, 'val2': 2})

    data = response.json()
    assert data['args'] == {'val1': '4', 'val2': '2'}
    assert data['url'] == 'https://httpbin.org/get?val1=4&val2=2'


def test_head():
    response = requests_curl.head('https://www.google.com/')
    assert response.status_code == 200
    assert response.content == None  # TODO: requests sets b''
    assert 'date' in response.headers
    assert 'expires' in response.headers

