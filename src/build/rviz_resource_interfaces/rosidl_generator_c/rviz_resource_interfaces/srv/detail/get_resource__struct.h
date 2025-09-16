// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from rviz_resource_interfaces:srv/GetResource.idl
// generated code does not contain a copyright notice

#ifndef RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__STRUCT_H_
#define RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'path'
// Member 'etag'
#include "rosidl_runtime_c/string.h"

/// Struct defined in srv/GetResource in the package rviz_resource_interfaces.
typedef struct rviz_resource_interfaces__srv__GetResource_Request
{
  rosidl_runtime_c__String path;
  /// HTTP-style ETag value for the requested resource.
  /// See: https://en.wikipedia.org/wiki/HTTP_ETag
  ///
  /// If this value is empty, then the server shall respond with the current
  /// version of the resource and the ETag value, if it can be loaded.
  /// If this value is not-empty, then the server may respond with a new resource
  /// and ETag value.
  /// However, if the ETag value for the resource has not changed, then the server
  /// may respond with NOT_MODIFIED as the status_code, similar to
  /// "HTTP 304: Not Modified".
  /// See: https://en.wikipedia.org/wiki/List_of_HTTP_status_codes#304
  /// The server may also ignore this value and always send the current version
  /// of the resource and its ETag value, if caching is not implemented.
  rosidl_runtime_c__String etag;
} rviz_resource_interfaces__srv__GetResource_Request;

// Struct for a sequence of rviz_resource_interfaces__srv__GetResource_Request.
typedef struct rviz_resource_interfaces__srv__GetResource_Request__Sequence
{
  rviz_resource_interfaces__srv__GetResource_Request * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} rviz_resource_interfaces__srv__GetResource_Request__Sequence;


// Constants defined in the message

/// Constant 'ERROR'.
/**
  * An unspecified error occurred, check the error_reason string.
 */
enum
{
  rviz_resource_interfaces__srv__GetResource_Response__ERROR = 0l
};

/// Constant 'OK'.
/**
  * The request was successful, etag and body will be set with valid values,
  * though etag may be empty.
  * The error_reason will be empty.
 */
enum
{
  rviz_resource_interfaces__srv__GetResource_Response__OK = 1l
};

/// Constant 'NOT_MODIFIED'.
/**
  * The request was successful, but the etag value has not changed.
  * The etag value will be set to the requested etag value, but the body value
  * will be empty.
  * The error_reason should also be empty.
 */
enum
{
  rviz_resource_interfaces__srv__GetResource_Response__NOT_MODIFIED = 2l
};

// Include directives for member types
// Member 'error_reason'
// Member 'expanded_path'
// Member 'etag'
// already included above
// #include "rosidl_runtime_c/string.h"
// Member 'body'
#include "rosidl_runtime_c/primitives_sequence.h"

/// Struct defined in srv/GetResource in the package rviz_resource_interfaces.
typedef struct rviz_resource_interfaces__srv__GetResource_Response
{
  /// Status code for the request, can be one of the above options.
  int32_t status_code;
  /// Optionally set error reason string.
  rosidl_runtime_c__String error_reason;
  /// Expanded path, which may or may not be different from the given path.
  /// The Service may expand, extend, or otherwise further qualify the path as it
  /// resolves it, any of which would be reflected in this expanded path.
  rosidl_runtime_c__String expanded_path;
  /// HTTP-style ETag value for the requested resource.
  /// See: https://en.wikipedia.org/wiki/HTTP_ETag
  ///
  /// As with the HTTP ETag, the value is unspecified, but it is described as:
  ///
  /// > Common methods of ETag generation include using a collision-resistant hash
  /// > function of the resource's content, a hash of the last modification
  /// > timestamp, or even just a revision number.
  ///
  /// This value may be empty if the server does not implement cache checking.
  ///
  /// This can be sent on subsequent requests to avoid getting the same unchanged
  /// resource multiple times.
  rosidl_runtime_c__String etag;
  /// Opaque value of the resource.
  rosidl_runtime_c__uint8__Sequence body;
} rviz_resource_interfaces__srv__GetResource_Response;

// Struct for a sequence of rviz_resource_interfaces__srv__GetResource_Response.
typedef struct rviz_resource_interfaces__srv__GetResource_Response__Sequence
{
  rviz_resource_interfaces__srv__GetResource_Response * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} rviz_resource_interfaces__srv__GetResource_Response__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__STRUCT_H_
