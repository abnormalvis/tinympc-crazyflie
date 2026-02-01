# TinyMPC Crazyflie 固件编译记录

## 日期
2026年2月1日

## 编译环境
- **操作系统**: Ubuntu 20.04 LTS (Linux)
- **编译器**: gcc-arm-none-eabi 9.2.1
- **Python版本**: Python 3.8

## 完成的工作

### 1. 安装编译依赖

#### ARM 交叉编译工具链
```bash
sudo apt install -y gcc-arm-none-eabi
```

安装的包：
- binutils-arm-none-eabi (2.34-4ubuntu1+13ubuntu1)
- gcc-arm-none-eabi (15:9-2019-q4-0ubuntu1)
- libnewlib-arm-none-eabi (3.3.0-0ubuntu1)
- libnewlib-dev (3.3.0-0ubuntu1)
- libstdc++-arm-none-eabi-newlib (15:9-2019-q4-0ubuntu1+12build2)

### 2. 解决编译问题

#### 问题：缺少 estimator 函数定义
编译时出现链接错误，缺少以下函数：
- `estimatorOutOfTreeInit`
- `estimatorOutOfTreeTest`
- `estimatorOutOfTree`

#### 解决方案：创建 Out-of-Tree Estimator 实现

**新建文件**: `src/estimator_oot.c`

实现了一个基于 sensfusion6 的简单状态估计器，提供：
- 姿态估计（Roll, Pitch, Yaw）
- 四元数姿态表示
- 加速度数据传递

**修改文件**: `src/Kbuild`

添加编译目标：
```makefile
obj-y += estimator_oot.o
```

### 3. 编译成功

#### 编译命令
```bash
make -j$(nproc)
```

#### 生成的固件文件
- `build/cf2.bin` (293 KB) - 二进制固件文件
- `build/cf2.hex` (823 KB) - HEX 格式固件文件
- `build/cf2.elf` (26 MB) - ELF 格式（含调试信息）
- `build/cf2.map` (4.7 MB) - 内存映射文件

#### 资源使用情况
- **Flash**: 299,104 / 1,032,192 字节 (29%) - **剩余 733,088 字节**
- **RAM**: 104,560 / 131,072 字节 (80%) - 剩余 26,512 字节
- **CCM**: 62,328 / 65,536 字节 (95%) - 剩余 3,208 字节

### 4. 安装 Crazyflie 工具

#### Python 虚拟环境
创建独立的 Python 虚拟环境避免依赖冲突：
```bash
python3 -m venv venv_cfclient
source venv_cfclient/bin/activate
```

#### 安装的包
- `cflib` (0.1.27) - Crazyflie Python 库
- `cfloader` (0.1.4) - 固件烧录工具
- `numpy` (1.24.4)
- `scipy` (1.10.1)
- `pyusb` (1.2.1)
- `libusb-package` (1.0.26.3)

## 固件烧录指南

### 方法 1：使用 make cload（推荐）
```bash
CLOAD_CMDS="-w radio://0/80/2M/E7E7E7E7E7" make cload
```

### 方法 2：使用 cfloader
```bash
source venv_cfclient/bin/activate
cfloader flash build/cf2.bin stm32-fw -w radio://0/80/2M/E7E7E7E7E7
```

### 准备工作
1. 插入 **CrazyRadio PA** USB 适配器
2. 打开 **Crazyflie** 无人机电源
3. 确保无人机在信号范围内

### 手动进入 Bootloader 模式
如果自动烧录失败，长按电源按钮 3 秒，直到指示灯快速闪烁。

### USB 权限设置（如需要）
```bash
sudo groupadd plugdev
sudo usermod -a -G plugdev $USER
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="1915", ATTRS{idProduct}=="7777", MODE="0664", GROUP="plugdev"' | sudo tee /etc/udev/rules.d/99-crazyradio.rules
sudo udevadm control --reload-rules
```

## 编译警告

### Eigen 对齐警告
编译过程中出现多个 Eigen 库的对齐警告（`-Wattributes`），这是由于 ARM Cortex-M4F 平台的内存对齐限制。这些警告不影响功能，已通过 `-Wno-error` 标志处理。

### C++ 兼容性警告
- ISO C++ 字符串常量转换警告（`-Wwrite-strings`）
- 未使用变量警告
- memcpy 使用警告

这些警告已知且不影响固件运行。

## 项目结构变更

### 新增文件
```
src/
├── estimator_oot.c          # Out-of-tree 状态估计器实现
└── Kbuild                   # 已修改：添加 estimator_oot.o
```

### 虚拟环境
```
venv_cfclient/               # Python 虚拟环境（用于 Crazyflie 工具）
```

## 技术细节

### Estimator 实现
- 使用 sensfusion6 进行姿态融合
- 更新频率：250 Hz (ATTITUDE_UPDATE_RATE)
- 数据源：陀螺仪、加速度计、气压计
- 输出：欧拉角（Roll, Pitch, Yaw）和四元数

### 控制器配置
- 控制器：TinyMPC (Out-of-tree controller)
- 估计器：Out-of-tree estimator（基于 sensfusion6）
- 平台：Crazyflie 2.X

## 下一步

1. **测试固件**：烧录到 Crazyflie 并测试基本功能
2. **参数调优**：根据实际飞行测试调整 TinyMPC 控制器参数
3. **性能验证**：测试控制性能和资源使用情况
4. **日志记录**：使用 Crazyflie 日志系统记录运行数据

## 参考资料
- [TinyMPC Crazyflie README](README.md)
- [Bitcraze Crazyflie Firmware Documentation](https://www.bitcraze.io/documentation/repository/crazyflie-firmware/master/)
- [TinyMPC Library](https://github.com/TinyMPC/TinyMPC)
