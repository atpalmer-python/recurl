import requests
import requests_curl


adapter = requests_curl.CurlEasyAdapter()
session = requests.Session()

session.mount('https://', adapter)

response = session.get('https://www.google.com/')

print('apparent_encoding:', response.apparent_encoding)
print('content[0:255]:', response.content[0:255])
print('cookies:', response.cookies)
print('elapsed:', response.elapsed)
print('encoding:', response.encoding)
print('headers:', response.headers)  # TODO: set as dict
print('history:', response.history)
print('is_permanent_redirect:', response.is_permanent_redirect)
print('is_redirect:', response.is_redirect)
print('links:', response.links)
print('next:', response.next)
# print('ok:', response.ok)  # TODO: needs status_code set
print('raw:', response.raw)
print('reason:', response.reason)
print('request:', response.request)
print('status_code:', response.status_code)
print('text[0:255]:', response.text[0:255])
print('url:', response.url)

