# training_diagnostics

Учебный пакет для ROS 2 Jazzy. Нода публикует heartbeat, принимает текстовые
команды, позволяет включать и выключать публикацию сервисом и загружает частоту
из YAML.

## Интерфейсы

- publisher: `/training/heartbeat` (`std_msgs/msg/String`);
- subscriber: `/training/command` (`std_msgs/msg/String`);
- service: `/training/enable` (`std_srvs/srv/SetBool`);
- parameter: `publish_rate` (`double`, по умолчанию `2.0` Гц).

Имена топиков и сервиса в коде относительные. Выше указаны полные имена при
запуске без namespace; при заданном namespace он добавляется перед `training/`.

Heartbeat имеет вид `RUNNING count=1`. Счётчик увеличивается только при
фактической публикации.
Во время отключения таймер продолжает работать, но сообщения не публикуются,
а счётчик не увеличивается. Команды пока только выводятся в лог и не меняют
состояние ноды.

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

Допустимы конечные числа типа `double` от **0.1 до 100.0 Гц включительно**.
Используйте `5.0`, а не целое `5`: тип параметра проверяется строго.
Ноль, отрицательные значения, `NaN`, бесконечность и значения вне диапазона
отклоняются.

При неправильной начальной частоте нода завершает запуск с ошибкой и ненулевым
кодом возврата. Автоматической подмены значения на `2.0` нет. При динамическом
изменении некорректное значение отклоняется, а прежняя частота сохраняется.
Таймер обновляется после успешного принятия параметра; повторная установка
того же значения не пересоздаёт его. Изменение через `ros2 param set` не
перезаписывает YAML и действует только до завершения ноды.

Пример проверки отклонения значения:

```bash
ros2 param set /training_diagnostics publish_rate 0.0
ros2 param get /training_diagnostics publish_rate
```

Первая команда должна сообщить об отказе, вторая — показать прежнее значение.

## Запуск нескольких экземпляров

В разных терминалах с подключённым окружением workspace выполните:

```bash
# Терминал 1
ros2 run training_diagnostics diagnostics_node --ros-args \
  -r __ns:=/robot1 -p publish_rate:=2.0
```

```bash
# Терминал 2
ros2 run training_diagnostics diagnostics_node --ros-args \
  -r __ns:=/robot2 -p publish_rate:=5.0
```

Эти команды запускают executable напрямую и задают частоту аргументами,
без загрузки YAML из launch-файла. Имена нод будут
`/robot1/training_diagnostics` и `/robot2/training_diagnostics`, а интерфейсы
получат соответствующие префиксы. Например:

```bash
ros2 topic echo /robot1/training/heartbeat
ros2 service call /robot1/training/enable std_srvs/srv/SetBool "{data: false}"
ros2 param get /robot2/training_diagnostics publish_rate
```

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
6. Установить `publish_rate` в `5.0` и проверить частоту через `topic hz`.
7. Попробовать установить `0.0`: параметр должен быть отклонён, частота — сохранена.

## Git

Если пакет ещё не находится в репозитории:

```bash
cd ~/ros2_ws/src/training_diagnostics
git init
git add .
git commit -m "Add training diagnostics ROS 2 package"
```
