#include <random>
#include <string>
#include <vector>

#include <XmlRpcValue.h>
#include <ros/protobuffer_traits.h>
#include <ros/serialization_protobuffer.h>
#include <ros/ros.h>

#include "publish_info.pb.h"
#include "system_status.pb.h"

#include "ros_protobuf_bridge/protobuf_utils.h"

namespace {

std::vector<std::string> LoadCoreNames(const ros::NodeHandle &nh) {
  XmlRpc::XmlRpcValue param_value;
  if (!nh.getParam("core_names", param_value)) {
    return {"cpu0", "cpu1", "cpu2", "cpu3"};
  }

  std::vector<std::string> result;
  if (param_value.getType() == XmlRpc::XmlRpcValue::TypeArray) {
    for (int i = 0; i < param_value.size(); ++i) {
      if (param_value[i].getType() == XmlRpc::XmlRpcValue::TypeString) {
        result.emplace_back(static_cast<std::string>(param_value[i]));
      }
    }
  }

  if (result.empty()) {
    result.push_back("cpu0");
  }
  return result;
}

} // namespace

int main(int argc, char **argv) {
  ros::init(argc, argv, "system_status_talker");
  ros::NodeHandle nh("~");

  double publish_rate = 2.0;
  nh.param("publish_rate", publish_rate, publish_rate);
  std::vector<std::string> cores = LoadCoreNames(nh);

  ros::NodeHandle global_nh;
  ros::Publisher pub =
      global_nh.advertise<superbai::sample::SystemStatus>("/system_status", 10);

  ROS_INFO_STREAM("Advertising /system_status as protobuf message\n"
                  << ros_protobuf_bridge::DescribeMessageType<
                         superbai::sample::SystemStatus>());

  std::mt19937 rng(std::random_device{}());
  std::uniform_real_distribution<double> load_dist(0.0, 1.0);
  std::uniform_real_distribution<double> temp_dist(20.0, 90.0);

  superbai::sample::PublishInfo publish_info;
  publish_info.set_name("system_status_talker");
  publish_info.set_message_type("SystemStatus");
  publish_info.set_publish_msg("Synthetic system status feed");

  ros::Rate loop_rate(publish_rate);
  while (ros::ok()) {
    superbai::sample::SystemStatus status;
    status.set_hostname(ros::this_node::getName());
    status.set_uptime_sec(ros::Time::now().sec);
    status.set_healthy(true);
    *status.mutable_last_publish() = publish_info;

    status.clear_cores();
    for (const auto &core_name : cores) {
      auto *core = status.add_cores();
      core->set_name(core_name);
      core->set_load(load_dist(rng));
    }

    status.clear_sensors();
    auto *temp = status.add_sensors();
    temp->set_name("board_temperature");
    temp->set_unit("C");
    temp->set_value(temp_dist(rng));

    auto *voltage = status.add_sensors();
    voltage->set_name("battery_voltage");
    voltage->set_unit("V");
    voltage->set_value(11.1 + load_dist(rng));

    pub.publish(status);
    ROS_DEBUG_STREAM("Published system status: " << status.DebugString());

    ros::spinOnce();
    loop_rate.sleep();
  }

  return 0;
}

