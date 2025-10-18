# ROS Protobuf Bridge

将 Google Protocol Buffers 消息无缝集成到 ROS `roscpp` 发布/订阅管线中的示例项目。仓库通过重写 ROS 的 `message_traits` 与
`serialization` 扩展点，使任何继承自 `google::protobuf::Message` 的类型都能像原生 ROS 消息一样被 `ros::Publisher` 和
`ros::Subscriber` 处理。项目还提供 Docker 开发环境、示例节点、JSON 工具库和测试用例，帮助你快速在工程中复用这一能力。

## 核心特性

- **Protobuf 消息即 ROS 消息**：为所有 Protobuf 类型实现 `DataType`、`MD5Sum`、`Definition` 等 traits，无需编写 `.msg`
  文件即可在 ROS 中传输自定义消息。
- **专用序列化桥接**：自定义 `Serializer` 模板，将 Protobuf 对象编码/解码为 ROS 所需的字节流，保证话题通信兼容性。
- **JSON ⇄ Protobuf 工具库**：`ros_protobuf_bridge_utils` 提供 JSON 序列化、反序列化与参数加载函数，便于与外部系统对接。
- **丰富的演示节点**：包括 `pb_talker`、`pb_listener`、`system_status_talker`、`pb_json_bridge`、`pb_topic_logger` 等，覆盖发布、
  转换和落盘场景。
- **可复用的构建产物**：`sample_protos` 中的 `.proto` 文件通过 `protobuf_generate` 自动生成 C++ 代码，示例节点展示了如何链接和发布。
- **开箱即用的 Docker 环境**：提供 ROS Noetic + Protobuf + Abseil 的镜像脚本与进入容器的工具，方便在隔离环境中体验项目。

## 仓库结构

```
.
├── CMakeLists.txt                 # 顶层 CMake 构建脚本
├── docker/                        # Dockerfile、构建与运行脚本
├── include/ros_protobuf_bridge/   # JSON 工具库头文件
├── message_traits/                # 自定义 message_traits 头文件与安装逻辑
├── message_serialization/         # 自定义序列化实现及安装逻辑
├── pb_talker.cpp / pb_listener.cpp# 基础 Protobuf 发布/订阅示例
├── system_status_talker.cpp       # 发布复杂 SystemStatus 消息的节点
├── pb_json_bridge.cpp             # 将 JSON 文本桥接为 Protobuf 消息
├── pb_topic_logger.cpp            # 订阅任意 Protobuf 主题并保存为 JSONL
├── sample_protos/                 # 示例 protobuf 定义与生成规则
├── src/                           # JSON 工具库实现
├── talker.cpp / listener.cpp      # 对照用的 std_msgs 示例
├── test/                          # 模板/Proto 行为测试
└── docs/                          # 其他文档与示例笔记
```

## 环境依赖

- **操作系统**：建议 Ubuntu 20.04。
- **编译工具链**：CMake ≥ 3.10、GCC ≥ 9。
- **ROS**：`ros-noetic-desktop-full` 或兼容安装，需具备 `catkin` 工作空间。
- **第三方库**：`protobuf`（含 `protoc`）、`abseil`。

Docker 镜像会自动安装上述依赖；本地手动安装可参考 `docker/build/ros_x86.dockerfile` 中的指令。

## 快速开始

### 使用 Docker

1. 构建镜像（默认标签 `ros_protobuf:noetic`）：
   ```bash
   cd docker/build
   docker build --network host -t ros_protobuf:noetic -f ros_x86.dockerfile .
   ```
2. 启动容器并挂载当前仓库：
   ```bash
   cd ../scripts
   ./ros_docker_run.sh
   ```
   容器名称默认为 `ros_noetic_proto`，代码挂载到 `/work`。
3. 进入容器：
   ```bash
   ./ros_docker_into.sh
   ```
   进入后执行 `source /opt/ros/noetic/setup.bash && source /work/devel/setup.bash` 初始化环境。

### 本地编译

1. 安装 ROS Noetic 与 Protobuf、Abseil 等依赖，确保 `source /opt/ros/noetic/setup.bash` 已加入 `~/.bashrc`。
2. 克隆仓库并在根目录执行：
   ```bash
   mkdir -p build
   cd build
   cmake ..
   make -j$(nproc)
   ```
   构建流程会生成 `roscpp_proto_serialization` 静态库、示例节点和测试程序，并将自定义头文件复制到 `/opt/ros/noetic/include/ros/`。
   如无写权限，可修改 `message_traits/CMakeLists.txt` 和 `message_serialization/CMakeLists.txt` 中的安装路径为本地可写目录。

## 添加新的 Protobuf 类型

1. 在 `sample_protos/` 中新增 `.proto` 文件，并在同目录 `CMakeLists.txt` 的 `PROTO_FILES` 列表中声明。
2. 执行 `catkin_make` 或顶层 `cmake` 构建，`protobuf_generate` 会自动生成对应的 C++ 源文件并链接到 `pb_proto` 库。
3. 在你的 ROS 节点中包含生成的头文件，像 `pb_talker` 或 `system_status_talker` 一样直接发布 Protobuf 消息。
4. 订阅端可通过 `ros::MessageEvent<T>` 访问原始对象；如需转换为 JSON，可调用 `ros_protobuf_bridge_utils::MessageToJsonString`。

## 示例工作流

以下示例假设你已完成构建并执行 `source devel/setup.bash`：

1. 启动 ROS 主节点：
   ```bash
   roscore &
   ```
2. 发布基础 `PublishInfo` 消息：
   ```bash
   rosrun ros_protobuf_bridge pb_talker
   ```
3. 订阅并查看消息内容：
   ```bash
   rosrun ros_protobuf_bridge pb_listener
   ```
   终端会打印收到的 Protobuf 字段及自动生成的消息描述信息。
4. 体验复杂结构的 `SystemStatus` 发布：
   ```bash
   rosrun ros_protobuf_bridge system_status_talker _publish_rate:=5.0
   ```
5. 将 JSON 文本桥接为 Protobuf：
   ```bash
   rosrun ros_protobuf_bridge pb_json_bridge _input_topic:=json_commands _target_type:=SystemStatus
   rostopic pub /pb_json_bridge/json_commands std_msgs/String '{"data": "{\\"hostname\\":\\"edge-01\\",\\"uptimeSec\\":123}"}'
   ```
   `pb_json_bridge` 会把 JSON 转换成目标 Protobuf 并发布到命名空间下的输出话题。
6. 将 Protobuf 消息落盘为 JSONL：
   ```bash
   rosrun ros_protobuf_bridge pb_topic_logger _topic:=/system_status _message_type:=SystemStatus _output_path:=/tmp/status.jsonl _pretty:=true
   ```
   日志器会把消息序列化为 JSON 行文本，便于离线分析。
7. 需要对照原生 ROS 消息时，可运行 `talker`/`listener` 示例了解差异。

## 测试

构建后会在 `build` 目录生成若干测试可执行文件，例如 `test_proto`、`test_sfinae`。可通过以下命令运行全部测试：
```bash
ctest --output-on-failure
```

## 常见问题排查

- **缺少写权限**：若无法向 `/opt/ros/noetic/include/ros/` 写入，可将自定义 traits 与序列化头文件安装到本地目录，并在编译时通过
  `-I` 指定额外的包含路径。
- **Protobuf 版本不匹配**：确保系统中的 `protoc` 与链接时使用的 `libprotobuf` 来自同一版本。
- **ROS 环境未初始化**：若 `rosrun` 提示包不存在，请确认已执行 `source devel/setup.bash` 并在包含 `devel` 目录的工作空间内。
- **JSON 解析失败**：`ParseJsonIntoMessage` 默认忽略未知字段，如需严格校验可调整 `JsonParseOptions` 设置。

## 许可证

项目基于 BSD-3-Clause 许可证发布，详情参见 [LICENSE.txt](LICENSE.txt)。
