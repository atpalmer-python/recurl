import pytest
import requests
import recurl


def test_bad_url():
    request = requests.PreparedRequest()
    request.__dict__.update(method='GET', url='nowhere')
    with pytest.raises(requests.exceptions.ConnectionError):
        response = recurl.CurlEasyAdapter().send(request)


def test_bad_http_version_value():
    with pytest.raises(ValueError):
        recurl.CurlEasyAdapter(http_version='abc')


def test_bad_http_version_type():
    with pytest.raises(TypeError):
        recurl.CurlEasyAdapter(http_version=1.1)


def test_bad_maxconnects_type():
    with pytest.raises(TypeError):
        recurl.CurlEasyAdapter(maxconnects=1.1)


def test_close():
    adapter = recurl.CurlEasyAdapter()
    adapter.close()
    assert True  # just testing this method exists and can be called without raising


def test_google():
    import datetime

    response = recurl.get('https://www.google.com/', http_version='1.1')

    assert response.apparent_encoding == 'windows-1250'
    # assert response.cookies == None  # TODO
    assert isinstance(response.elapsed, datetime.timedelta)
    assert response.encoding == 'ISO-8859-1'
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


def test_head():
    response = recurl.head('https://www.google.com/')
    assert response.status_code == 200
    assert response.content == None  # TODO: requests sets b''
    assert 'date' in response.headers
    assert 'expires' in response.headers


def test_get():
    response = recurl.get('https://httpbin.org/get', params={'val1': 4, 'val2': 2})
    data = response.json()
    assert data['args'] == {'val1': '4', 'val2': '2'}
    assert data['url'] == 'https://httpbin.org/get?val1=4&val2=2'


def test_post():
    response = recurl.post('https://httpbin.org/post', data={'val1': 4, 'val2': 2})
    data = response.json()
    assert data['form'] == {'val1': '4', 'val2': '2'}


def test_put():
    response = recurl.put('https://httpbin.org/put', data={'val1': 4, 'val2': 2})
    data = response.json()
    assert data['form'] == {'val1': '4', 'val2': '2'}


def test_patch():
    response = recurl.patch('https://httpbin.org/patch', data={'val1': 4, 'val2': 2})
    data = response.json()
    assert data['form'] == {'val1': '4', 'val2': '2'}


def test_delete():
    response = recurl.delete('https://httpbin.org/delete')
    assert response.ok


def test_headers():
    response = recurl.get('https://httpbin.org/anything', headers={'x-myheader': 'some-value'})
    data = response.json()
    assert requests.structures.CaseInsensitiveDict(data['headers']).get('x-myheader') == 'some-value'


def test_timeout():
    '''
    NOTE:
    libcurl does not provide separate error codes corresponding
    to requests's ConnectionTimeout and ReadTimeout.
    We can only catch the generic Timeout exception.
    '''
    with pytest.raises(requests.exceptions.Timeout):
        recurl.get('https://httpbin.org/delay/2', timeout=1)
    response = recurl.get('https://httpbin.org/delay/2')
    assert response.ok


def test_timeout_tuple():
    with pytest.raises(requests.exceptions.Timeout):
        recurl.get('https://httpbin.org/delay/2', timeout=(1, 1))


def test_proxy():
    with pytest.raises(requests.exceptions.ProxyError):
        recurl.get('https://www.google.com/', proxies={'https': 'http://proxy.google.com:1080'})
    response = recurl.get('https://www.google.com/')
    assert response.ok


def test_cert():
    with pytest.raises(requests.exceptions.RequestException):  # TODO: OSError?
        recurl.get('https://www.google.com/', cert='/badcert')
    response = recurl.get('https://www.google.com/')
    assert response.ok

