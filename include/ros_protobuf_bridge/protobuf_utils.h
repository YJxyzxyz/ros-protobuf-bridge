#pragma once

#include <string>

#include <google/protobuf/message.h>
#include <ros/message_traits.h>
#include <ros/node_handle.h>

namespace ros_protobuf_bridge {

/**
 * @brief Convert a protobuf message to a JSON string.
 *
 * @param message Source protobuf message.
 * @param pretty  Whether to pretty print with indentation.
 * @return std::string JSON representation of the message.
 */
std::string ToJsonString(const google::protobuf::Message &message,
                         bool pretty = false);

/**
 * @brief Parse a JSON string into a protobuf message instance.
 *
 * @param json    JSON payload.
 * @param message Target protobuf message which will be overwritten.
 * @param error   Optional pointer that receives detailed error description when
 *                parsing fails.
 * @return true   Parsing succeeded.
 * @return false  Parsing failed, message remains untouched.
 */
bool ParseJsonIntoMessage(const std::string &json,
                          google::protobuf::Message *message,
                          std::string *error = nullptr);

/**
 * @brief Load a JSON parameter from the ROS parameter server and parse it into
 *        the provided protobuf message.
 *
 * The parameter is expected to be a JSON string. When missing, the function
 * returns false and leaves the message untouched. Any parsing errors are
 * reported via ROS logging utilities.
 */
bool LoadJsonParam(const ros::NodeHandle &nh, const std::string &param_name,
                   google::protobuf::Message *message);

/**
 * @brief Produce a human readable description of the protobuf message type.
 */
template <typename ProtobufMessage> std::string DescribeMessageType() {
  std::string summary;
  summary +=
      std::string("datatype: ") +
      ros::message_traits::DataType<ProtobufMessage>::value() + "\n";
  summary += std::string("md5: ") +
             ros::message_traits::MD5Sum<ProtobufMessage>::value() + "\n";
  summary += "definition:\n";
  summary += ros::message_traits::Definition<ProtobufMessage>::value();
  return summary;
}

} // namespace ros_protobuf_bridge

