# ROS Protobuf Bridge

一个将 Google Protocol Buffers 消息无缝集成到 ROS `roscpp` 发布/订阅管线中的示例项目。本仓库通过重写 ROS 的 `message_traits` 与 `serialization` 扩展点，让任意继承自 `google::protobuf::Message` 的类型都能像原生 ROS 消息一样被 `ros::Publisher` 和 `ros::Subscriber` 处理，并提供了完整的 Docker 环境、示例节点与测试用例，帮助你快速在项目中复用这一能力。

## 功能亮点

- **Protobuf 消息即 ROS 消息**：针对所有 Protobuf 类型实现了 `DataType`、`MD5Sum`、`Definition` 等 traits，自动暴露消息名称与描述信息。无需再编写 `.msg` 文件即可在 ROS 中传输自定义消息。【F:message_traits/include/ros/protobuffer_traits.h†L33-L108】
- **二进制序列化桥接**：提供自定义 `Serializer` 模板，负责将 Protobuf 消息编码为 ROS 期望的字节流，并在订阅端进行解码。【F:message_serialization/include/ros/serialization_protobuffer.h†L24-L74】
- **JSON ⇄ Protobuf 工具库**：新增 `ros_protobuf_bridge_utils` 库，提供 JSON 序列化、反序列化以及参数加载辅助函数，便于与 Web 服务或配置管理系统对接。【F:include/ros_protobuf_bridge/protobuf_utils.h†L9-L59】【F:src/protobuf_utils.cpp†L9-L66】
- **多主题演示节点**：在原有 `pb_talker`/`pb_listener` 基础上增加 `system_status_talker`、`pb_json_bridge`、`pb_topic_logger` 等节点，覆盖发布、转换、落盘全链路场景。【F:system_status_talker.cpp†L1-L114】【F:pb_json_bridge.cpp†L1-L82】【F:pb_topic_logger.cpp†L1-L84】
- **可复用的构建产物**：`pb_talker`、`pb_listener` 演示了如何发布/订阅 Protobuf 消息，`sample_protos` 中的 `.proto` 文件通过 `protobuf_generate` 自动生成 C++ 代码并链接到示例节点。【F:pb_talker.cpp†L1-L34】【F:pb_listener.cpp†L26-L72】【F:sample_protos/CMakeLists.txt†L1-L23】
- **开箱即用的 Docker 开发环境**：提供 ROS Noetic + Protobuf + Abseil 的镜像构建脚本与容器启动脚本，方便在隔离环境中体验项目。【F:docker/build/ros_x86.dockerfile†L1-L44】【F:docker/scripts/ros_docker_run.sh†L1-L34】

## 仓库结构

```
.
├── CMakeLists.txt           # 顶层 CMake 构建脚本
├── docker/                  # Dockerfile、构建与运行脚本
├── message_traits/          # 自定义 message_traits 头文件
├── message_serialization/   # 自定义序列化库与头文件
├── sample_protos/           # 示例 protobuf 定义与生成规则
├── pb_talker.cpp / pb_listener.cpp  # Protobuf 版本的发布/订阅示例
├── system_status_talker.cpp        # 发布复杂 SystemStatus 消息的节点
├── pb_json_bridge.cpp              # 将 JSON 文本桥接为 Protobuf 消息
├── pb_topic_logger.cpp             # 订阅任意 Protobuf 主题并保存为 JSONL
├── talker.cpp / listener.cpp        # 原生 std_msgs 版本的参考示例
├── include/ros_protobuf_bridge/    # JSON 辅助函数等通用头文件
├── src/                            # 工具库实现
├── test/                           # 模板/Proto 行为测试
└── docs/                           # 其他文档与示例笔记
```

## 环境依赖

- **基础环境**：Ubuntu 20.04、CMake ≥ 3.10、GCC ≥ 9、`ros-noetic-desktop-full`。【F:docker/build/ros_x86.dockerfile†L1-L25】
- **开发工具**：`protobuf`（自动安装在 Docker 镜像内）、`abseil`、`catkin`。
- **ROS 工作空间**：工程假定位于 `/work` 并依赖 `/opt/ros/noetic` 的头文件与库。自定义 traits/serialization 会在构建时复制到该目录下，需保证有写入权限。【F:message_traits/CMakeLists.txt†L4-L11】【F:message_serialization/CMakeLists.txt†L12-L28】

## 快速开始

### 使用 Docker 体验
1. **构建镜像**（默认标签 `ros_protobuf:noetic`）：
   ```bash
   cd docker/build
   docker build --network host -t ros_protobuf:noetic -f ros_x86.dockerfile .
   ```
2. **启动容器**：
   ```bash
   cd docker/scripts
   ./ros_docker_run.sh
   ```
   脚本会创建名为 `ros_noetic_proto` 的容器，并将当前仓库挂载到容器内 `/work`。【F:docker/scripts/ros_docker_run.sh†L1-L34】
3. **进入容器**：
   ```bash
   ./ros_docker_into.sh
   ```
   默认会以 `root` 用户进入容器 Shell。【F:docker/scripts/ros_docker_into.sh†L1-L9】

### 手动在本地编译
1. 安装 ROS Noetic 及其依赖，确保 `source /opt/ros/noetic/setup.bash` 已写入 `~/.bashrc`。
2. 安装 Protobuf 与 Abseil（可参考 Dockerfile 中的脚本执行方式）。
3. 获取本仓库代码后执行：
   ```bash
   mkdir -p build
   cd build
   cmake ..
   make -j$(nproc)
   ```
   构建过程会编译 `roscpp_proto_serialization` 静态库、示例节点和测试程序，并把自定义头文件复制到 `/opt/ros/noetic/include/ros/`。如无写权限，可将相关路径改为本地 include 目录。

## Protobuf 集成指南

1. 在 `sample_protos/` 中添加新的 `.proto` 文件，并在 `CMakeLists.txt` 的 `PROTO_FILES` 列表中声明。例如仓库已经提供了 `PublishInfo` 与嵌套字段丰富的 `SystemStatus` 定义，可作为参考。【F:sample_protos/CMakeLists.txt†L7-L18】【F:sample_protos/system_status.proto†L1-L23】
2. 通过 `protobuf_generate` 自动生成 C++ 文件后，`pb_proto` 库会将其暴露给上层可执行文件使用。【F:sample_protos/CMakeLists.txt†L14-L23】
3. 在你的 ROS 节点中包含生成的头文件，并像示例中的 `pb_talker` 或 `system_status_talker` 一样直接发布 Protobuf 消息即可。【F:pb_talker.cpp†L1-L34】【F:system_status_talker.cpp†L1-L114】
4. 订阅端同样可以直接接收该消息，并通过 `MessageEvent` 访问原始 Protobuf 对象进行调试或打印。若希望把收到的消息转存为 JSON，可直接使用 `pb_topic_logger`。【F:pb_listener.cpp†L34-L72】【F:pb_topic_logger.cpp†L1-L84】
5. 如果需要把外部系统的 JSON 配置转换为 Protobuf，可调用 `ros_protobuf_bridge_utils` 提供的 `ParseJsonIntoMessage` 或 `LoadJsonParam` 辅助函数。【F:include/ros_protobuf_bridge/protobuf_utils.h†L9-L59】【F:src/protobuf_utils.cpp†L9-L66】

## 运行示例

1. **初始化环境**：
   ```bash
   source /opt/ros/noetic/setup.bash
   source /work/devel/setup.bash   # Docker 容器中已在 ~/.bashrc 配置
   ```
2. **发布基础 PublishInfo 消息**：
   ```bash
   roscore &
   rosrun myproject pb_talker
   ```
3. **订阅并查看消息描述信息**：
   ```bash
   rosrun myproject pb_listener
   ```
   Listener 会输出收到的 `PublishInfo` 消息内容以及自动生成的消息描述信息。【F:pb_listener.cpp†L34-L72】
4. **体验复杂结构的 SystemStatus 发布**：
   ```bash
   rosrun myproject system_status_talker _publish_rate:=5.0
   ```
   节点会按设定频率发布随机生成的系统监控数据，方便调试多字段消息。【F:system_status_talker.cpp†L41-L112】
5. **将 JSON 文本桥接为 Protobuf**：
   ```bash
   rosrun myproject pb_json_bridge _input_topic:=json_commands _target_type:=SystemStatus
   rostopic pub /pb_json_bridge/json_commands std_msgs/String '{"data": "{\\"hostname\\":\\"edge-01\\",\\"uptimeSec\\":123}"}'
   ```
   `pb_json_bridge` 会自动把 JSON 转换成目标 Protobuf 并发布到命名空间下的输出话题。【F:pb_json_bridge.cpp†L15-L82】
6. **将 Protobuf 消息落盘为 JSONL**：
   ```bash
   rosrun myproject pb_topic_logger _topic:=/system_status _message_type:=SystemStatus _output_path:=/tmp/status.jsonl _pretty:=true
   ```
   日志器会把消息序列化为 JSON 行文本，包含类型描述信息，便于离线分析。【F:pb_topic_logger.cpp†L1-L84】
7. （可选）运行 `talker`/`listener` 观察传统 `std_msgs/String` 的行为，以便对比差异。【F:talker.cpp†L1-L118】

## 测试与调试

- 构建目录下会生成 `test_temple`、`test_sfinae`、`test_proto` 等可执行文件，用于验证模板推导和 Protobuf 行为。【F:test/CMakeLists.txt†L5-L18】
- 如需在容器外进行调试，可利用 Dockerfile 中已经安装的 `gdb`、`vim` 等工具。【F:docker/build/ros_x86.dockerfile†L6-L31】

## 常见问题

- **缺少复制权限**：默认会将自定义头文件复制到 `/opt/ros/noetic/include/ros/`。若权限不足，可修改 `message_traits/CMakeLists.txt` 与 `message_serialization/CMakeLists.txt`，将目标路径替换成自定义的包含目录并在编译时加入 `-I` 搜索路径。
- **Protobuf 版本冲突**：确保系统安装的 `protoc` 版本与 `protobuf::libprotobuf` 库一致，避免运行时 `Symbol not found`。
- **ROS 环境未初始化**：若运行 `rosrun` 提示包不存在，请确认已经执行 `source devel/setup.bash` 并位于含有 `devel` 目录的工作空间内。
- **JSON 序列化失败**：`ros_protobuf_bridge_utils` 默认忽略未知字段。若需要强校验，可在调用时自行修改 `ParseJsonIntoMessage` 中的 `JsonParseOptions` 设置。【F:src/protobuf_utils.cpp†L28-L56】

## 许可证

项目基于 BSD-3-Clause 许可证分发。详细内容见 [LICENSE.txt](LICENSE.txt)。
