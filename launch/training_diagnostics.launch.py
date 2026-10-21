from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    # Путь к установленной копии YAML (share/training_diagnostics/config).
    package_share = Path(get_package_share_directory("training_diagnostics"))
    parameters_file = package_share / "config" / "diagnostics.yaml"

    return LaunchDescription([
        Node(
            package="training_diagnostics",
            executable="diagnostics_node",
            name="training_diagnostics",
            output="screen",
            parameters=[str(parameters_file)],
        )
    ])
