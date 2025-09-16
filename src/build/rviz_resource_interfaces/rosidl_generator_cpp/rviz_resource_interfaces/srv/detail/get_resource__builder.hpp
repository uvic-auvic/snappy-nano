// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from rviz_resource_interfaces:srv/GetResource.idl
// generated code does not contain a copyright notice

#ifndef RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__BUILDER_HPP_
#define RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "rviz_resource_interfaces/srv/detail/get_resource__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace rviz_resource_interfaces
{

namespace srv
{

namespace builder
{

class Init_GetResource_Request_etag
{
public:
  explicit Init_GetResource_Request_etag(::rviz_resource_interfaces::srv::GetResource_Request & msg)
  : msg_(msg)
  {}
  ::rviz_resource_interfaces::srv::GetResource_Request etag(::rviz_resource_interfaces::srv::GetResource_Request::_etag_type arg)
  {
    msg_.etag = std::move(arg);
    return std::move(msg_);
  }

private:
  ::rviz_resource_interfaces::srv::GetResource_Request msg_;
};

class Init_GetResource_Request_path
{
public:
  Init_GetResource_Request_path()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_GetResource_Request_etag path(::rviz_resource_interfaces::srv::GetResource_Request::_path_type arg)
  {
    msg_.path = std::move(arg);
    return Init_GetResource_Request_etag(msg_);
  }

private:
  ::rviz_resource_interfaces::srv::GetResource_Request msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::rviz_resource_interfaces::srv::GetResource_Request>()
{
  return rviz_resource_interfaces::srv::builder::Init_GetResource_Request_path();
}

}  // namespace rviz_resource_interfaces


namespace rviz_resource_interfaces
{

namespace srv
{

namespace builder
{

class Init_GetResource_Response_body
{
public:
  explicit Init_GetResource_Response_body(::rviz_resource_interfaces::srv::GetResource_Response & msg)
  : msg_(msg)
  {}
  ::rviz_resource_interfaces::srv::GetResource_Response body(::rviz_resource_interfaces::srv::GetResource_Response::_body_type arg)
  {
    msg_.body = std::move(arg);
    return std::move(msg_);
  }

private:
  ::rviz_resource_interfaces::srv::GetResource_Response msg_;
};

class Init_GetResource_Response_etag
{
public:
  explicit Init_GetResource_Response_etag(::rviz_resource_interfaces::srv::GetResource_Response & msg)
  : msg_(msg)
  {}
  Init_GetResource_Response_body etag(::rviz_resource_interfaces::srv::GetResource_Response::_etag_type arg)
  {
    msg_.etag = std::move(arg);
    return Init_GetResource_Response_body(msg_);
  }

private:
  ::rviz_resource_interfaces::srv::GetResource_Response msg_;
};

class Init_GetResource_Response_expanded_path
{
public:
  explicit Init_GetResource_Response_expanded_path(::rviz_resource_interfaces::srv::GetResource_Response & msg)
  : msg_(msg)
  {}
  Init_GetResource_Response_etag expanded_path(::rviz_resource_interfaces::srv::GetResource_Response::_expanded_path_type arg)
  {
    msg_.expanded_path = std::move(arg);
    return Init_GetResource_Response_etag(msg_);
  }

private:
  ::rviz_resource_interfaces::srv::GetResource_Response msg_;
};

class Init_GetResource_Response_error_reason
{
public:
  explicit Init_GetResource_Response_error_reason(::rviz_resource_interfaces::srv::GetResource_Response & msg)
  : msg_(msg)
  {}
  Init_GetResource_Response_expanded_path error_reason(::rviz_resource_interfaces::srv::GetResource_Response::_error_reason_type arg)
  {
    msg_.error_reason = std::move(arg);
    return Init_GetResource_Response_expanded_path(msg_);
  }

private:
  ::rviz_resource_interfaces::srv::GetResource_Response msg_;
};

class Init_GetResource_Response_status_code
{
public:
  Init_GetResource_Response_status_code()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_GetResource_Response_error_reason status_code(::rviz_resource_interfaces::srv::GetResource_Response::_status_code_type arg)
  {
    msg_.status_code = std::move(arg);
    return Init_GetResource_Response_error_reason(msg_);
  }

private:
  ::rviz_resource_interfaces::srv::GetResource_Response msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::rviz_resource_interfaces::srv::GetResource_Response>()
{
  return rviz_resource_interfaces::srv::builder::Init_GetResource_Response_status_code();
}

}  // namespace rviz_resource_interfaces

#endif  // RVIZ_RESOURCE_INTERFACES__SRV__DETAIL__GET_RESOURCE__BUILDER_HPP_
