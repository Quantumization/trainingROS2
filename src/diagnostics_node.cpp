#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "rcl_interfaces/msg/set_parameters_result.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/set_bool.hpp"

// Учебная нода: heartbeat, приём текстовых команд, сервис вкл/выкл и параметр частоты.
class DiagnosticsNode : public rclcpp::Node
{
public:
  explicit DiagnosticsNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Node("training_diagnostics", options)
  {
    // Проверяем начальную конфигурацию до создания таймера.
    publish_rate_ = declare_parameter<double>("publish_rate", 2.0);
    if (!is_valid_rate(publish_rate_)) {
      throw std::invalid_argument(kRateError);
    }

    heartbeat_publisher_ = create_publisher<std_msgs::msg::String>(
      "training/heartbeat", 10);

    // Пока только логируем содержимое; команды не меняют состояние ноды.
    command_subscription_ = create_subscription<std_msgs::msg::String>(
      "training/command", 10,
      [this](std_msgs::msg::String::ConstSharedPtr message) {
        on_command(*message);
      });

    // request->data == true  — публиковать heartbeat
    // request->data == false — таймер крутится, но сообщения не отправляются
    enable_service_ = create_service<std_srvs::srv::SetBool>(
      "training/enable",
      [this](
        const std_srvs::srv::SetBool::Request::SharedPtr request,
        std_srvs::srv::SetBool::Response::SharedPtr response)
      {
        on_enable(*request, *response);
      });

    parameter_callback_handle_ = add_on_set_parameters_callback(
      [this](const std::vector<rclcpp::Parameter> & parameters) {
        return validate_parameters(parameters);
      });
    // Применяем изменения только после успешного принятия параметров ROS 2.
    post_parameter_callback_handle_ = add_post_set_parameters_callback(
      [this](const std::vector<rclcpp::Parameter> & parameters) {
        apply_parameters(parameters);
      });

    create_heartbeat_timer();
    RCLCPP_INFO(get_logger(), "Diagnostics node started at %.2f Hz", publish_rate_);
  }

private:
  static constexpr double kMinRateHz = 0.1;
  static constexpr double kMaxRateHz = 100.0;
  static constexpr const char * kRateError =
    "publish_rate must be a finite double in [0.1, 100.0] Hz";

  static bool is_valid_rate(double rate)
  {
    return std::isfinite(rate) && rate >= kMinRateHz && rate <= kMaxRateHz;
  }

  void on_command(const std_msgs::msg::String & message)
  {
    RCLCPP_INFO(get_logger(), "Command received: '%s'", message.data.c_str());
  }

  void on_enable(
    const std_srvs::srv::SetBool::Request & request,
    std_srvs::srv::SetBool::Response & response)
  {
    enabled_ = request.data;
    response.success = true;
    response.message = enabled_ ? "Heartbeat publication enabled" :
      "Heartbeat publication disabled";
    RCLCPP_INFO(get_logger(), "%s", response.message.c_str());
  }

  void publish_heartbeat()
  {
    if (!enabled_) {
      return;
    }
    std_msgs::msg::String message;
    message.data = "RUNNING count=" + std::to_string(message_count_ + 1);
    heartbeat_publisher_->publish(message);
    ++message_count_;
    RCLCPP_DEBUG(get_logger(), "Published: '%s'", message.data.c_str());
  }

  // Пересоздаёт wall-timer под текущий publish_rate_.
  // Сначала создаём замену, затем явно отменяем старый таймер.
  void create_heartbeat_timer()
  {
    const auto period = std::chrono::duration<double>(1.0 / publish_rate_);
    auto new_timer = create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(period),
      [this]() {publish_heartbeat();});
    if (heartbeat_timer_) {
      heartbeat_timer_->cancel();
    }
    heartbeat_timer_ = new_timer;
  }

  // Колбэк ROS 2: можно отклонить некорректное значение (result.successful = false).
  rcl_interfaces::msg::SetParametersResult validate_parameters(
    const std::vector<rclcpp::Parameter> & parameters) const
  {
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;

    for (const auto & parameter : parameters) {
      if (parameter.get_name() == "publish_rate") {
        if (parameter.get_type() != rclcpp::ParameterType::PARAMETER_DOUBLE ||
          !is_valid_rate(parameter.as_double()))
        {
          result.successful = false;
          result.reason = kRateError;
          return result;
        }
      }
    }
    return result;
  }

  void apply_parameters(const std::vector<rclcpp::Parameter> & parameters)
  {
    for (const auto & parameter : parameters) {
      if (parameter.get_name() == "publish_rate" && parameter.as_double() != publish_rate_) {
        publish_rate_ = parameter.as_double();
        create_heartbeat_timer();
        RCLCPP_INFO(get_logger(), "Publication rate changed to %.2f Hz", publish_rate_);
      }
    }
  }

  // Обработчики выполняются последовательно в стандартной callback group.
  double publish_rate_{2.0};
  bool enabled_{true};
  std::uint64_t message_count_{0};
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr heartbeat_publisher_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr command_subscription_;
  rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr enable_service_;
  rclcpp::TimerBase::SharedPtr heartbeat_timer_;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameter_callback_handle_;
  rclcpp::node_interfaces::PostSetParametersCallbackHandle::SharedPtr post_parameter_callback_handle_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  int exit_code = 0;
  try {
    rclcpp::spin(std::make_shared<DiagnosticsNode>());
  } catch (const std::exception & error) {
    RCLCPP_ERROR(rclcpp::get_logger("training_diagnostics"), "%s", error.what());
    exit_code = 1;
  }
  rclcpp::shutdown();
  return exit_code;
}
