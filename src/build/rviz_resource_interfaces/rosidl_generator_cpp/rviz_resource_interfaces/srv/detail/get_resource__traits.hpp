// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from rviz_resource_interfaces:srv/GetResource.idl
// generated code does not contain a copyright notice

#ifndef RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__TRAITS_HPP_
#define RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "rviz_resource_interfaces/srv/detail/get_resource__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace rviz_resource_interfaces
{

namespace srv
{

inline void to_flow_style_yaml(
  const GetResource_Request & msg,
  std::ostream & out)
{
  out << "{";
  // member: path
  {
    out << "path: ";
    rosidl_generator_traits::value_to_yaml(msg.path, out);
    out << ", ";
  }

  // member: etag
  {
    out << "etag: ";
    rosidl_generator_traits::value_to_yaml(msg.etag, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const GetResource_Request & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: path
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "path: ";
    rosidl_generator_traits::value_to_yaml(msg.path, out);
    out << "\n";
  }

  // member: etag
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "etag: ";
    rosidl_generator_traits::value_to_yaml(msg.etag, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const GetResource_Request & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace srv

}  // namespace rviz_resource_interfaces

namespace rosidl_generator_traits
{

[[deprecated("use rviz_resource_interfaces::srv::to_block_style_yaml() instead")]]
inline void to_yaml(
  const rviz_resource_interfaces::srv::GetResource_Request & msg,
  std::ostream & out, size_t indentation = 0)
{
  rviz_resource_interfaces::srv::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use rviz_resource_interfaces::srv::to_yaml() instead")]]
inline std::string to_yaml(const rviz_resource_interfaces::srv::GetResource_Request & msg)
{
  return rviz_resource_interfaces::srv::to_yaml(msg);
}

template<>
inline const char * data_type<rviz_resource_interfaces::srv::GetResource_Request>()
{
  return "rviz_resource_interfaces::srv::GetResource_Request";
}

template<>
inline const char * name<rviz_resource_interfaces::srv::GetResource_Request>()
{
  return "rviz_resource_interfaces/srv/GetResource_Request";
}

template<>
struct has_fixed_size<rviz_resource_interfaces::srv::GetResource_Request>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<rviz_resource_interfaces::srv::GetResource_Request>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<rviz_resource_interfaces::srv::GetResource_Request>
  : std::true_type {};

}  // namespace rosidl_generator_traits

namespace rviz_resource_interfaces
{

namespace srv
{

inline void to_flow_style_yaml(
  const GetResource_Response & msg,
  std::ostream & out)
{
  out << "{";
  // member: status_code
  {
    out << "status_code: ";
    rosidl_generator_traits::value_to_yaml(msg.status_code, out);
    out << ", ";
  }

  // member: error_reason
  {
    out << "error_reason: ";
    rosidl_generator_traits::value_to_yaml(msg.error_reason, out);
    out << ", ";
  }

  // member: expanded_path
  {
    out << "expanded_path: ";
    rosidl_generator_traits::value_to_yaml(msg.expanded_path, out);
    out << ", ";
  }

  // member: etag
  {
    out << "etag: ";
    rosidl_generator_traits::value_to_yaml(msg.etag, out);
    out << ", ";
  }

  // member: body
  {
    if (msg.body.size() == 0) {
      out << "body: []";
    } else {
      out << "body: [";
      size_t pending_items = msg.body.size();
      for (auto item : msg.body) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const GetResource_Response & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: status_code
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "status_code: ";
    rosidl_generator_traits::value_to_yaml(msg.status_code, out);
    out << "\n";
  }

  // member: error_reason
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "error_reason: ";
    rosidl_generator_traits::value_to_yaml(msg.error_reason, out);
    out << "\n";
  }

  // member: expanded_path
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "expanded_path: ";
    rosidl_generator_traits::value_to_yaml(msg.expanded_path, out);
    out << "\n";
  }

  // member: etag
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "etag: ";
    rosidl_generator_traits::value_to_yaml(msg.etag, out);
    out << "\n";
  }

  // member: body
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.body.size() == 0) {
      out << "body: []\n";
    } else {
      out << "body:\n";
      for (auto item : msg.body) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const GetResource_Response & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace srv

}  // namespace rviz_resource_interfaces

namespace rosidl_generator_traits
{

[[deprecated("use rviz_resource_interfaces::srv::to_block_style_yaml() instead")]]
inline void to_yaml(
  const rviz_resource_interfaces::srv::GetResource_Response & msg,
  std::ostream & out, size_t indentation = 0)
{
  rviz_resource_interfaces::srv::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use rviz_resource_interfaces::srv::to_yaml() instead")]]
inline std::string to_yaml(const rviz_resource_interfaces::srv::GetResource_Response & msg)
{
  return rviz_resource_interfaces::srv::to_yaml(msg);
}

template<>
inline const char * data_type<rviz_resource_interfaces::srv::GetResource_Response>()
{
  return "rviz_resource_interfaces::srv::GetResource_Response";
}

template<>
inline const char * name<rviz_resource_interfaces::srv::GetResource_Response>()
{
  return "rviz_resource_interfaces/srv/GetResource_Response";
}

template<>
struct has_fixed_size<rviz_resource_interfaces::srv::GetResource_Response>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<rviz_resource_interfaces::srv::GetResource_Response>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<rviz_resource_interfaces::srv::GetResource_Response>
  : std::true_type {};

}  // namespace rosidl_generator_traits

namespace rosidl_generator_traits
{

template<>
inline const char * data_type<rviz_resource_interfaces::srv::GetResource>()
{
  return "rviz_resource_interfaces::srv::GetResource";
}

template<>
inline const char * name<rviz_resource_interfaces::srv::GetResource>()
{
  return "rviz_resource_interfaces/srv/GetResource";
}

template<>
struct has_fixed_size<rviz_resource_interfaces::srv::GetResource>
  : std::integral_constant<
    bool,
    has_fixed_size<rviz_resource_interfaces::srv::GetResource_Request>::value &&
    has_fixed_size<rviz_resource_interfaces::srv::GetResource_Response>::value
  >
{
};

template<>
struct has_bounded_size<rviz_resource_interfaces::srv::GetResource>
  : std::integral_constant<
    bool,
    has_bounded_size<rviz_resource_interfaces::srv::GetResource_Request>::value &&
    has_bounded_size<rviz_resource_interfaces::srv::GetResource_Response>::value
  >
{
};

template<>
struct is_service<rviz_resource_interfaces::srv::GetResource>
  : std::true_type
{
};

template<>
struct is_service_request<rviz_resource_interfaces::srv::GetResource_Request>
  : std::true_type
{
};

template<>
struct is_service_response<rviz_resource_interfaces::srv::GetResource_Response>
  : std::true_type
{
};

}  // namespace rosidl_generator_traits

#endif  // RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__TRAITS_HPP_
