import requests
import requests_curl


def test_google():
    import datetime

    # TODO: simplify this setup code
    adapter = requests_curl.CurlEasyAdapter(http_version='1.1')
    session = requests.Session()
    session.mount('https://', adapter)

    response = session.get('https://www.google.com/')

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
    adapter = requests_curl.CurlEasyAdapter()
    session = requests.Session()
    session.mount('https://', adapter)

    response = session.get('https://httpbin.org/get', params={'val1': 4, 'val2': 2})

    data = response.json()
    assert data['args'] == {'val1': '4', 'val2': '2'}
    assert data['url'] == 'https://httpbin.org/get?val1=4&val2=2'

