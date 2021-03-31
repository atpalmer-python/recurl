# Recurl: Curl adapter for requests

```
import requests
import recurl


adapter = recurl.CurlEasyAdapter()
session = requests.Session()
session.mount('https://', adapter)

response = session.get('https://www.google.com/')

print(response.ok)
```

...or...

```
import recurl


session = recurl.Session()
response = session.get('https://www.google.com/')
print(response.ok)
```

...or...

```
import recurl


response = recurl.get('https://www.google.com/')
print(response.ok)
```

# TODO

* Cookies/CookieJars
* Exceptions
* Streaming downloads
* Streaming uploads
* History
* Links
* Hooks
* Parse HTTP line continuations
* Audit API vs. requests
* Extend API for CURL-specific features?
* CURL "multi" sessions (async requests)

