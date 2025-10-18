#include "ros_protobuf_bridge/protobuf_utils.h"

#include <google/protobuf/util/json_util.h>
#include <ros/ros.h>

namespace ros_protobuf_bridge {

std::string ToJsonString(const google::protobuf::Message &message,
                         bool pretty) {
  google::protobuf::util::JsonPrintOptions options;
  options.add_whitespace = pretty;
  options.always_print_primitive_fields = true;
  std::string json_output;
  auto status =
      google::protobuf::util::MessageToJsonString(message, &json_output, options);
  if (!status.ok()) {
    ROS_ERROR_STREAM("Failed to serialize protobuf message to JSON: "
                     << status.ToString());
    return {};
  }
  return json_output;
}

bool ParseJsonIntoMessage(const std::string &json,
                          google::protobuf::Message *message,
                          std::string *error) {
  if (!message) {
    if (error) {
      *error = "Target protobuf message pointer is null";
    }
    return false;
  }

  google::protobuf::util::JsonParseOptions options;
  options.ignore_unknown_fields = true;
  google::protobuf::Message *mutable_msg = message->New();
  auto status = google::protobuf::util::JsonStringToMessage(json, mutable_msg,
                                                            options);
  if (!status.ok()) {
    if (error) {
      *error = status.ToString();
    }
    delete mutable_msg;
    return false;
  }

  message->CopyFrom(*mutable_msg);
  delete mutable_msg;
  return true;
}

bool LoadJsonParam(const ros::NodeHandle &nh, const std::string &param_name,
                   google::protobuf::Message *message) {
  std::string json_payload;
  if (!nh.getParam(param_name, json_payload)) {
    ROS_DEBUG_STREAM("Parameter '" << param_name << "' not found");
    return false;
  }

  std::string error_message;
  if (!ParseJsonIntoMessage(json_payload, message, &error_message)) {
    ROS_ERROR_STREAM("Failed to parse parameter '" << param_name
                                                   << "' into protobuf: "
                                                   << error_message);
    return false;
  }

  ROS_INFO_STREAM("Loaded protobuf parameters from '" << param_name << "'.");
  return true;
}

} // namespace ros_protobuf_bridge

