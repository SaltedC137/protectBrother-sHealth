# Screen Light Adjust

Screen Light Adjust 把显示器亮度/色温控制和定时休息提醒放在一个 Qt 桌面程序中。预设会同时保存这两类设置，选中一个预设后可一键应用。
## 功能
- X11：使用 `xrandr` 逐输出调整亮度和色温。
- Wayland：使用 `redshift`，缺少时尝试 `gammastep`；色温和软件亮度按所有输出统一调整。
- 休息提醒：工作周期结束后，在 Qt 检测到的每个屏幕上显示相同的自定义提示，休息结束后自动开始下一轮。
- 预设：保存显示器、同步选项、提醒启用状态、工作/休息时长和提示语；设置默认预设后，每次启动都会自动应用。
- 托盘：通过 `QSystemTrayIcon` 注册图标，支持 XEmbed 或 StatusNotifierItem 的桌面托盘服务。
## 桌面环境
X11 下显示控制按屏幕输出分别应用。Wayland 没有通用的屏幕色温接口；显示调整依赖合成器提供 Redshift 所需的 gamma-control 后端，Wayland 页面提供全局显示设置。休息提醒使用 Qt 的每屏全屏窗口；窗口置顶、抢焦点等行为仍由合成器策略决定。
Qt 托盘图标需要桌面提供兼容的托盘宿主。i3 环境应启用 i3bar 托盘（例如 `tray_output primary`）；如果桌面没有托盘服务，程序会留在主窗口而不会悄悄隐藏，关闭窗口会退出应用。
Wayland 色温依赖合成器提供的 gamma-control 协议；无法连接时，应用会在状态栏显示错误。协议不可用的合成器仍可使用定时休息提醒。

## 项目结构

- `include/`：显示控制、统一预设、休息计时和多屏提醒的公共头文件。
- `src/`：应用入口和功能实现；功能代码由 `screen-light-adjust-core` 目标统一编译。
- `tests/`：统一预设、计时和多屏提醒检查。
- `resources/`：桌面入口、图标和 Qt 插件路径配置。
- `cmake/`：DEB 安装与卸载脚本。

## 构建
需要 Qt 6 Widgets、CMake 3.16 及 C++17 编译器。设置 `QT_HOME_PATH` 指向 Qt 安装目录。
```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$QT_HOME_PATH" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```
## 安装包
在 Debian/Ubuntu 上构建 DEB：
```bash
cmake --build build --target deb
```
在 Fedora/RHEL 等 RPM 系统上构建 RPM：
```bash
cmake --build build --target rpm
```
DEB/RPM 元数据声明 `xrandr`、`redshift` 和 `gammastep` 运行依赖，由系统包管理器在安装时一并安装。RPM 构建需要 `rpmbuild`；Qt Wayland 运行需要对应平台插件。
## 配置文件
- 应用配置：`$XDG_CONFIG_HOME/screen-light-adjust/settings.json`；未设置 `XDG_CONFIG_HOME` 时使用 `~/.config/screen-light-adjust/settings.json`。
- 预设：同目录下的 `presets/<名称>.json`。
