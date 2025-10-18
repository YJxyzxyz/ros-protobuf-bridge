# ROS Protobuf Bridge

该项目通过重写 ROS 的 `message_traits` 与 `serialization` 扩展点，让任意继承自 `google::protobuf::Message` 的类型都能像原生 ROS 消息一样工作，并附带完整的示例节点、测试用例与 Docker 环境，帮助你在现有工程中快速复用这套机制。

---

## 功能概览
- **透明的消息 traits**：为所有 Protobuf 类型提供 `DataType`、`MD5Sum`、`Definition` 等 traits 实现，`ros::Publisher` 与 `ros::Subscriber` 可以像处理普通 `.msg` 一样处理 Protobuf 对象。
- **专用序列化器**：通过模板化的 `Serializer`，在发布端自动将 Protobuf 对象编码为字节流，在订阅端解码回原始对象。
- **JSON ↔︎ Protobuf 工具库**：`ros_protobuf_bridge_utils` 提供 JSON 解析、序列化以及 ROS 参数加载函数，简化与 Web 服务或配置系统对接的流程。
- **丰富的演示节点**：`pb_talker`/`pb_listener` 演示基本的发布订阅，`system_status_talker`、`pb_json_bridge`、`pb_topic_logger` 覆盖复杂消息生成、JSON 转换与落盘等场景。
- **即用型 Docker 环境**：提供构建脚本与容器入口脚本，预装 ROS Noetic、Protobuf、Abseil 等依赖，适合快速体验或在隔离环境中开发。

---

## 仓库结构
```
.
├── CMakeLists.txt                  # 顶层构建脚本
├── docker/                         # Dockerfile 与辅助脚本
├── include/ros_protobuf_bridge/    # JSON 工具等公共头文件
├── message_traits/                 # 自定义 message_traits 头文件与安装脚本
├── message_serialization/          # 自定义序列化库
├── sample_protos/                  # 示例 .proto 定义与生成规则
├── src/                            # JSON 工具库实现
├── test/                           # 单元/集成测试
├── pb_*.cpp / system_status_talker.cpp  # 示例节点源文件
├── talker.cpp / listener.cpp            # std_msgs 对照示例
└── docs/                           # 其他文档
```

---

## 依赖要求
- **操作系统**：Ubuntu 20.04（如使用 Docker 镜像则已满足）。
- **工具链**：CMake ≥ 3.10、GCC ≥ 9。
- **ROS 组件**：`ros-noetic-desktop-full` 与 `catkin`。
- **第三方库**：`protobuf`、`abseil`。Dockerfile 中包含完整的安装过程，可作为本地搭建参考。
- **权限要求**：构建脚本会把自定义 traits/serialization 头文件复制到 `/opt/ros/noetic/include/ros/`。若无写权限，可修改对应 CMakeLists，将安装路径改为自定义目录并手动加入 include 搜索路径。

---

## 快速开始

### 方式一：使用 Docker
1. 构建镜像（默认标签 `ros_protobuf:noetic`）：
   ```bash
   cd docker/build
   docker build --network host -t ros_protobuf:noetic -f ros_x86.dockerfile .
   ```
2. 启动容器并挂载当前仓库：
   ```bash
   cd docker/scripts
   ./ros_docker_run.sh
   ```
   脚本会创建名为 `ros_noetic_proto` 的容器，并将仓库映射到容器内 `/work`。
3. 进入容器环境：
   ```bash
   ./ros_docker_into.sh
   ```
   容器中已默认 `source /opt/ros/noetic/setup.bash` 与 `/work/devel/setup.bash`。

### 方式二：在本机手动构建
1. 安装 ROS Noetic，确保 `source /opt/ros/noetic/setup.bash` 已写入 `~/.bashrc`。
2. 安装 Protobuf 与 Abseil。可参考 `docker/build/ros_x86.dockerfile` 中的安装指令。
3. 在仓库根目录执行：
   ```bash
   mkdir -p build
   cd build
   cmake ..
   make -j$(nproc)
   ```
   构建结果会生成示例节点、`roscpp_proto_serialization` 静态库以及测试可执行文件。

---

## 运行示例节点
1. 初始化 ROS 环境：
   ```bash
   source /opt/ros/noetic/setup.bash
   source /work/devel/setup.bash  # 若在 Docker 内已自动完成
   roscore &
   ```
2. 发布基础 `PublishInfo` Protobuf 消息：
   ```bash
   rosrun myproject pb_talker
   ```
3. 订阅并打印消息内容：
   ```bash
   rosrun myproject pb_listener
   ```
4. 发布复杂的系统状态：
   ```bash
   rosrun myproject system_status_talker _publish_rate:=5.0
   ```
5. 将 JSON 指令桥接为 Protobuf：
   ```bash
   rosrun myproject pb_json_bridge _input_topic:=json_commands _target_type:=SystemStatus
   rostopic pub /pb_json_bridge/json_commands std_msgs/String '{"data":"{\\"hostname\\":\\"edge-01\\",\\"uptimeSec\\":123}"}'
   ```
6. 订阅任意 Protobuf 主题并落盘为 JSONL：
   ```bash
   rosrun myproject pb_topic_logger \
     _topic:=/system_status \
     _message_type:=SystemStatus \
     _output_path:=/tmp/status.jsonl \
     _pretty:=true
   ```
7. 对照参考：运行 `talker`/`listener` 查看传统 `std_msgs/String` 行为。

---

## 自定义 Protobuf 工作流
1. 在 `sample_protos/` 新增 `.proto` 文件，并在 `CMakeLists.txt` 的 `PROTO_FILES` 列表中注册。
2. 运行 `catkin_make` 或 `cmake`/`make`，`protobuf_generate` 会在构建目录中生成对应的 `.pb.cc` 与 `.pb.h`。
3. 示例库 `pb_proto` 会将生成的源文件编译成可链接目标，节点源文件可直接包含生成头文件并发布/订阅新消息。
4. 如果需要在 JSON 与 Protobuf 之间转换，可调用 `ros_protobuf_bridge_utils` 中的 `ParseJsonIntoMessage`、`MessageToJsonString`、`LoadJsonParam` 等函数。
5. 构建完成后，新的消息类型无需编写 `.msg` 文件即可被 ROS 节点识别。

---

## 测试与调试
- 构建后可运行 `test_template`、`test_sfinae`、`test_proto` 等可执行文件，以验证 traits 推导、序列化行为和 Protobuf 解析。
- Docker 镜像预装 `gdb`、`valgrind`、`vim` 等工具，方便在容器内调试。
- 若需要在主机上调试，可借鉴 `docker/scripts/ros_docker_run.sh` 的挂载参数，在自定义容器或工作空间中复现同样的环境变量与卷映射。

---

## 常见问题
- **头文件安装失败**：如在复制到 `/opt/ros/noetic/include/ros/` 时遇到权限问题，可编辑 `message_traits/` 与 `message_serialization/` 下的 CMakeLists，将 `INSTALL_DIR` 改为具备写权限的路径，并在构建时通过 `-I` 添加到 include 搜索路径。
- **Protobuf 版本不匹配**：确保 `protoc` 命令与链接时使用的 `libprotobuf` 版本一致，避免运行时符号缺失。
- **找不到包**：执行 `rosrun` 前务必 `source devel/setup.bash`，并确认当前终端所在的工作空间包含编译后的 `devel` 目录。
- **JSON 解析报错**：默认解析选项会忽略未知字段。如需严格校验，可在 `ParseJsonIntoMessage` 中修改 `google::protobuf::util::JsonParseOptions`。

---

## 许可证
本项目基于 MIT 许可证发布，完整内容见 [LICENSE.txt](LICENSE.txt)。

