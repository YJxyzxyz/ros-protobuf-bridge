#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

#include <ros/protobuffer_traits.h>
#include <ros/ros.h>
#include <ros/serialization_protobuffer.h>

#include "publish_info.pb.h"
#include "system_status.pb.h"

#include "ros_protobuf_bridge/protobuf_utils.h"

namespace {

template <typename MessageT> class TopicLogger {
public:
  TopicLogger(const std::string &path, bool pretty)
      : output_path_(path), pretty_(pretty) {
    stream_.open(output_path_, std::ios::out | std::ios::app);
    if (!stream_.is_open()) {
      throw std::runtime_error("Unable to open log file: " + output_path_);
    }
    stream_ << "# " << ros_protobuf_bridge::DescribeMessageType<MessageT>()
            << "\n";
  }

  void operator()(const boost::shared_ptr<MessageT const> &msg) {
    const std::string json = ros_protobuf_bridge::ToJsonString(*msg, pretty_);
    if (json.empty()) {
      ROS_WARN_STREAM("Skipping message because JSON serialization failed.");
      return;
    }
    stream_ << json << "\n";
    stream_.flush();
  }

private:
  std::string output_path_;
  bool pretty_;
  std::ofstream stream_;
};

} // namespace

int main(int argc, char **argv) {
  ros::init(argc, argv, "pb_topic_logger");
  ros::NodeHandle nh("~");

  std::string topic = "/Sorbai";
  nh.param("topic", topic, topic);
  std::string type = "PublishInfo";
  nh.param("message_type", type, type);
  std::string output_path = "protobuf_log.jsonl";
  nh.param("output_path", output_path, output_path);
  bool pretty = false;
  nh.param("pretty", pretty, pretty);

  ROS_INFO_STREAM("Logging protobuf topic '" << topic << "' to '" << output_path
                                            << "' as type " << type);

  ros::Subscriber sub;
  try {
    if (type == "SystemStatus") {
      auto logger = std::make_shared<TopicLogger<superbai::sample::SystemStatus>>(
          output_path, pretty);
      sub = nh.subscribe<superbai::sample::SystemStatus>(
          topic, 10,
          [logger](const boost::shared_ptr<superbai::sample::SystemStatus const>
                       &msg) { (*logger)(msg); });
    } else {
      auto logger = std::make_shared<TopicLogger<superbai::sample::PublishInfo>>(
          output_path, pretty);
      sub = nh.subscribe<superbai::sample::PublishInfo>(
          topic, 10,
          [logger](const boost::shared_ptr<superbai::sample::PublishInfo const>
                       &msg) { (*logger)(msg); });
    }
  } catch (const std::exception &ex) {
    ROS_FATAL_STREAM("Failed to create TopicLogger: " << ex.what());
    return 1;
  }

  ros::spin();
  return 0;
}

