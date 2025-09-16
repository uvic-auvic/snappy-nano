

# An unspecified error occurred, check the error_reason string.
int32 ERROR=0

# The request was successful, etag and body will be set with valid values,
# though etag may be empty.
# The error_reason will be empty.
int32 OK=1

# The request was successful, but the etag value has not changed.
# The etag value will be set to the requested etag value, but the body value
# will be empty.
# The error_reason should also be empty.
int32 NOT_MODIFIED=2

# Status code for the request, can be one of the above options.
int32 status_code

# Optionally set error reason string.
string error_reason

# Expanded path, which may or may not be different from the given path.
# The Service may expand, extend, or otherwise further qualify the path as it
# resolves it, any of which would be reflected in this expanded path.
string expanded_path

# HTTP-style ETag value for the requested resource.
# See: https://en.wikipedia.org/wiki/HTTP_ETag
#
# As with the HTTP ETag, the value is unspecified, but it is described as:
#
# > Common methods of ETag generation include using a collision-resistant hash
# > function of the resource's content, a hash of the last modification
# > timestamp, or even just a revision number.
#
# This value may be empty if the server does not implement cache checking.
#
# This can be sent on subsequent requests to avoid getting the same unchanged
# resource multiple times.
string etag

# Opaque value of the resource.
uint8[] body
