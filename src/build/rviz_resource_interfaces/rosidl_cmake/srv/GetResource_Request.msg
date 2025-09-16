# Resource path of the requested resource.
string path

# HTTP-style ETag value for the requested resource.
# See: https://en.wikipedia.org/wiki/HTTP_ETag
#
# If this value is empty, then the server shall respond with the current
# version of the resource and the ETag value, if it can be loaded.
# If this value is not-empty, then the server may respond with a new resource
# and ETag value.
# However, if the ETag value for the resource has not changed, then the server
# may respond with NOT_MODIFIED as the status_code, similar to
# "HTTP 304: Not Modified".
# See: https://en.wikipedia.org/wiki/List_of_HTTP_status_codes#304
# The server may also ignore this value and always send the current version
# of the resource and its ETag value, if caching is not implemented.
string etag

