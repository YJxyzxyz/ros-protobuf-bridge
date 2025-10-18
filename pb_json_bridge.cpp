#include <functional>
#include <string>

#include <ros/protobuffer_traits.h>
#include <ros/ros.h>
#include <ros/serialization_protobuffer.h>
#include <std_msgs/String.h>

#include "publish_info.pb.h"
#include "system_status.pb.h"

#include "ros_protobuf_bridge/protobuf_utils.h"

namespace {

enum class TargetType { kPublishInfo, kSystemStatus };

TargetType ParseTargetType(const std::string &type_str) {
  if (type_str == "SystemStatus") {
    return TargetType::kSystemStatus;
  }
  return TargetType::kPublishInfo;
}

class JsonBridge {
public:
  JsonBridge(const ros::NodeHandle &nh, const std::string &target_type)
      : nh_(nh), target_type_(ParseTargetType(target_type)) {
    publisher_info_ = nh_.advertise<superbai::sample::PublishInfo>(
        "publish_info_output", 10, false);
    publisher_status_ = nh_.advertise<superbai::sample::SystemStatus>(
        "system_status_output", 10, false);
  }

  void HandleJson(const std_msgs::String::ConstPtr &msg) {
    switch (target_type_) {
    case TargetType::kPublishInfo: {
      superbai::sample::PublishInfo info;
      if (!ros_protobuf_bridge::ParseJsonIntoMessage(msg->data, &info)) {
        ROS_WARN_STREAM("Failed to decode PublishInfo JSON payload");
        return;
      }
      publisher_info_.publish(info);
      ROS_DEBUG_STREAM("Bridged JSON to PublishInfo: " << info.DebugString());
      break;
    }
    case TargetType::kSystemStatus: {
      superbai::sample::SystemStatus status;
      if (!ros_protobuf_bridge::ParseJsonIntoMessage(msg->data, &status)) {
        ROS_WARN_STREAM("Failed to decode SystemStatus JSON payload");
        return;
      }
      publisher_status_.publish(status);
      ROS_DEBUG_STREAM("Bridged JSON to SystemStatus: " << status.DebugString());
      break;
    }
    }
  }

private:
  ros::NodeHandle nh_;
  TargetType target_type_;
  ros::Publisher publisher_info_;
  ros::Publisher publisher_status_;
};

} // namespace

int main(int argc, char **argv) {
  ros::init(argc, argv, "pb_json_bridge");
  ros::NodeHandle nh("~");

  std::string input_topic = "json_input";
  nh.param("input_topic", input_topic, input_topic);
  std::string target_type = "PublishInfo";
  nh.param("target_type", target_type, target_type);

  JsonBridge bridge(nh, target_type);

  ros::Subscriber sub = nh.subscribe<std_msgs::String>(
      input_topic, 10, &JsonBridge::HandleJson, &bridge);

  ROS_INFO_STREAM("JSON bridge ready. Subscribing to '~" << input_topic
                                                        << "' expecting type "
                                                        << target_type);

  ros::spin();
  return 0;
}

