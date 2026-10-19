#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <cstdint>
#include <vector>

#include "rcl_interfaces/msg/set_parameters_result.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/set_bool.hpp"

class DiagnosticsNode : public rclcpp::Node
{
public:
  DiagnosticsNode()
  : Node("training_diagnostics"), enabled_(true), message_count_(0)
  {
    publish_rate_ = declare_parameter<double>("publish_rate", 2.0);
    if (publish_rate_ <= 0.0) {
      RCLCPP_WARN(get_logger(), "publish_rate must be positive; using 2.0 Hz");
      publish_rate_ = 2.0;
    }

    heartbeat_publisher_ = create_publisher<std_msgs::msg::String>(
      "/training/heartbeat", 10);

    command_subscription_ = create_subscription<std_msgs::msg::String>(
      "/training/command", 10,
      [this](const std_msgs::msg::String::SharedPtr message) {
        RCLCPP_INFO(get_logger(), "Command received: '%s'", message->data.c_str());
      });

    enable_service_ = create_service<std_srvs::srv::SetBool>(
      "/training/enable",
      [this](
        const std_srvs::srv::SetBool::Request::SharedPtr request,
        std_srvs::srv::SetBool::Response::SharedPtr response)
      {
        enabled_ = request->data;
        response->success = true;
        response->message = enabled_ ? "Heartbeat publication enabled" :
          "Heartbeat publication disabled";
        RCLCPP_INFO(get_logger(), "%s", response->message.c_str());
      });

    parameter_callback_handle_ = add_on_set_parameters_callback(
      std::bind(&DiagnosticsNode::on_parameters_changed, this, std::placeholders::_1));

    create_heartbeat_timer();
    RCLCPP_INFO(get_logger(), "Diagnostics node started at %.2f Hz", publish_rate_);
  }

private:
  void create_heartbeat_timer()
  {
    const auto period = std::chrono::duration<double>(1.0 / publish_rate_);
    heartbeat_timer_ = create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(period),
      [this]() {
        if (!enabled_) {
          return;
        }

        auto message = std_msgs::msg::String();
        message.data = "RUNNING count=" + std::to_string(++message_count_);
        heartbeat_publisher_->publish(message);
        RCLCPP_DEBUG(get_logger(), "Published: '%s'", message.data.c_str());
      });
  }

  rcl_interfaces::msg::SetParametersResult on_parameters_changed(
    const std::vector<rclcpp::Parameter> & parameters)
  {
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;

    for (const auto & parameter : parameters) {
      if (parameter.get_name() == "publish_rate") {
        if (parameter.get_type() != rclcpp::ParameterType::PARAMETER_DOUBLE ||
          parameter.as_double() <= 0.0)
        {
          result.successful = false;
          result.reason = "publish_rate must be a positive double";
          return result;
        }

        publish_rate_ = parameter.as_double();
        create_heartbeat_timer();
        RCLCPP_INFO(get_logger(), "Publication rate changed to %.2f Hz", publish_rate_);
      }
    }
    return result;
  }

  double publish_rate_;
  bool enabled_;
  std::uint64_t message_count_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr heartbeat_publisher_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr command_subscription_;
  rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr enable_service_;
  rclcpp::TimerBase::SharedPtr heartbeat_timer_;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameter_callback_handle_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DiagnosticsNode>());
  rclcpp::shutdown();
  return 0;
}
