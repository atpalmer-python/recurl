import requests
import requests_curl


adapter = requests_curl.CurlEasyAdapter()
session = requests.Session()

session.mount('https://', adapter)

response = session.get('https://www.google.com/')

print(response)
