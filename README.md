# training_diagnostics

Учебный пакет для ROS 2 Jazzy. Нода публикует heartbeat, принимает текстовые
команды, позволяет включать и выключать публикацию сервисом и загружает частоту
из YAML.

## Интерфейсы

- publisher: `/training/heartbeat` (`std_msgs/msg/String`);
- subscriber: `/training/command` (`std_msgs/msg/String`);
- service: `/training/enable` (`std_srvs/srv/SetBool`);
- parameter: `publish_rate` (`double`, по умолчанию `2.0` Гц).

Heartbeat имеет вид `RUNNING count=1`. Счётчик увеличивается только при
фактической публикации.

## Сборка

Поместите каталог пакета в `src` вашего workspace, затем выполните:

```bash
cd ~/ros2_ws
source /opt/ros/jazzy/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --packages-select training_diagnostics --symlink-install
source install/setup.bash
```

## Запуск

```bash
ros2 launch training_diagnostics training_diagnostics.launch.py
```

В отдельных терминалах (после `source ~/ros2_ws/install/setup.bash`):

```bash
ros2 topic echo /training/heartbeat
ros2 topic hz /training/heartbeat
ros2 topic pub --once /training/command std_msgs/msg/String "{data: 'CHECK'}"
```

Остановить и возобновить heartbeat:

```bash
ros2 service call /training/enable std_srvs/srv/SetBool "{data: false}"
ros2 service call /training/enable std_srvs/srv/SetBool "{data: true}"
```

## Параметры

Для постоянного изменения частоты отредактируйте
`config/diagnostics.yaml`, пересоберите пакет и снова запустите launch-файл.
При сборке с `--symlink-install` повторная сборка после редактирования обычно не
нужна.

Проверить и изменить параметр во время работы:

```bash
ros2 param get /training_diagnostics publish_rate
ros2 param set /training_diagnostics publish_rate 5.0
```

Значение должно быть положительным числом с плавающей точкой, например `5.0`.

## Граф нод

При запущенной ноде выполните:

```bash
rqt_graph
```

Чтобы увидеть subscriber-связь, одновременно запустите команду `ros2 topic pub`
без `--once` либо включите отображение всех топиков в `rqt_graph`.

## rosbag

Запись heartbeat (остановить сочетанием `Ctrl+C`):

```bash
ros2 bag record /training/heartbeat -o training_heartbeat_bag
```

Посмотреть сведения и воспроизвести запись:

```bash
ros2 bag info training_heartbeat_bag
ros2 bag play training_heartbeat_bag
```

Во время воспроизведения можно проверить данные командой:

```bash
ros2 topic echo /training/heartbeat
```

## Быстрая проверка результата

1. Запустить launch-файл и убедиться, что `topic echo` выводит счётчик.
2. `topic hz` должен показывать частоту, близкую к значению `publish_rate`.
3. Отправить `CHECK` и найти `Command received: 'CHECK'` в логе ноды.
4. Вызвать сервис с `false`: новые сообщения должны прекратиться.
5. Вызвать сервис с `true`: публикация должна продолжиться со следующего номера.

## Git

Если пакет ещё не находится в репозитории:

```bash
cd ~/ros2_ws/src/training_diagnostics
git init
git add .
git commit -m "Add training diagnostics ROS 2 package"
```
