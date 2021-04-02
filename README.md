# Recurl: Curl adapter for requests

## Usage

### Simple usage:

```
import recurl


response = recurl.get('https://www.google.com/')
print(response)  # requests.Response object
```

### Sessions:

```
import recurl


session = recurl.Session()  # requests.Session object with
                            # CurlEasyAdapter already mounted
response = session.get('https://www.google.com/')
...
```

### Verbose setup:

```
import requests
import recurl


adapter = recurl.CurlEasyAdapter()
session = requests.Session()
session.mount('https://', adapter)

response = session.get('https://www.google.com/')
...
```

## TODO

* Cookies/CookieJars
* Exceptions
* Streaming downloads
* Streaming uploads
* History
* Links
* Hooks
* Audit API vs. requests
* Extend API for CURL-specific features?
* CURL "multi" sessions (async requests)

