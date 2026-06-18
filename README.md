# 车载控制系统 (Vehicle Control System)

基于 STM32 + CAN 总线的车载温度监控与散热控制系统。

## 项目概述

本项目由三个 STM32 节点组成，通过 CAN 总线互联，实现车载温度采集、散热控制和状态显示功能：

| 模块 | 芯片 | 功能 |
|------|------|------|
| **主控** | STM32F407VETx | CAN 总线温度数据接收、超温判断与报警 |
| **检测** | STM32F103C8 | NTC 热敏电阻温度采集、PID 风扇调速、CAN 数据发送 |
| **交互** | STM32F103C8 | OLED 屏幕显示、CAN 数据收发、人机交互 |

## 系统架构

```
┌─────────────┐    CAN Bus (ID:0x002)    ┌─────────────┐
│   检测节点    │ ───────────────────────→ │   主控节点    │
│  STM32F103  │  温度数据 (float*100)     │  STM32F407  │
│             │                          │             │
│  NTC 测温   │                          │ 超温判断     │
│  PID 风扇   │                          │ 报警逻辑     │
└─────────────┘                          └──────┬──────┘
                                                │
                                                │ CAN Bus (ID:0x001)
                                                │ Warning 数据
                                                ↓
                                         ┌─────────────┐
                                         │   交互节点    │
                                         │  STM32F103  │
                                         │             │
                                         │ OLED 显示   │
                                         │ 状态交互     │
                                         └─────────────┘
```

## 各模块详解

### 1. 主控 (STM32F407VETx)

- **核心功能**：监听 CAN 总线上的温度数据，当温度超过阈值（38°C）连续 3 次时，通过 CAN 总线发送报警信号
- **外设**：CAN1、USART1（调试输出）
- **CAN ID**：发送 `0x001`（报警数据 `"Warning"`）
- **时钟**：HSI → PLL → 168MHz

**关键文件：**
- `主控/Core/Src/main.c` — 主程序，温度接收与超温判断逻辑
- `主控/Core/Src/can.c` — CAN 通信驱动
- `主控/Core/Src/usart.c` — 串口调试输出

### 2. 检测 (STM32F103C8)

- **核心功能**：通过 NTC 热敏电阻 + ADC 采集温度，使用 PID 算法控制 PWM 风扇转速，每 500ms 通过 CAN 发送温度数据
- **外设**：ADC1 + DMA、TIM2（PWM 输出 CH2）、CAN、USART1
- **CAN ID**：发送 `0x002`（温度数据，`float×100` 拆分为 2 字节）
- **时钟**：HSE → PLL → 72MHz

**PID 参数：**
| 参数 | 值 |
|------|-----|
| Kp | 25.0 |
| Ki | 0.1 |
| Kd | 0.0 |
| 目标温度 | 28°C |
| 输出上限 | 1000 |

**关键文件：**
- `检测/Core/Src/main.c` — 主程序，ADC 采集、温度计算、PID 控制
- `检测/Core/Src/pid_control.c` — PID 控制算法实现
- `检测/Core/Src/adc.c` — ADC + DMA 驱动
- `检测/Core/Src/tim.c` — PWM 定时器驱动

### 3. 交互 (STM32F103C8)

- **核心功能**：OLED 显示屏显示状态，CAN 数据收发测试
- **外设**：CAN、USART1、I2C（OLED）、GPIO
- **CAN ID**：发送 `0x001`，接收报警数据
- **时钟**：HSE → PLL → 72MHz

**关键文件：**
- `交互/Core/Src/main.c` — 主程序，OLED 显示与 CAN 通信
- `交互/Core/Src/OLED.c` — OLED 驱动（I2C）
- `交互/Core/Src/i2c.c` — I2C 驱动

## 开发环境

- **IDE**：Keil MDK-ARM (uVision)
- **HAL 库**：STM32CubeMX 生成
- **编译器**：ARM Compiler 5/6
- **调试器**：ST-Link / J-Link

## 烧录与运行

### 硬件连接

```
         CAN_H  ────────────────────
         CAN_L  ────────────────────
           │            │            │
       ┌───┴───┐   ┌───┴───┐   ┌───┴───┐
       │ 主控   │   │ 检测   │   │ 交互   │
       │ F407  │   │ F103  │   │ F103  │
       └───────┘   └───────┘   └───────┘
```

### 烧录步骤

1. 使用 Keil MDK 分别打开各模块的工程文件：
   - `主控/MDK-ARM/can_f407.uvprojx`
   - `检测/MDK-ARM/can.uvprojx`
   - `交互/MDK-ARM/can.uvprojx`
2. 编译工程（Build）
3. 通过 ST-Link 烧录至对应开发板
4. 也可以通过预编译的 `.hex` 文件直接烧录

### 调试

- 各模块通过 USART1 输出调试信息（波特率 115200）
- 使用串口助手连接对应开发板的串口即可查看运行日志

## 项目结构

```
车载控制/
├── 主控/                    # STM32F407 主控节点
│   ├── Core/
│   │   ├── Inc/             # 头文件 (can.h, gpio.h, usart.h, ...)
│   │   └── Src/             # 源文件 (main.c, can.c, usart.c, ...)
│   ├── Drivers/             # HAL 库 & CMSIS
│   └── MDK-ARM/             # Keil 工程文件
├── 检测/                    # STM32F103 检测节点
│   ├── Core/
│   │   ├── Inc/             # 头文件 (pid_control.h, adc.h, tim.h, ...)
│   │   └── Src/             # 源文件 (main.c, pid_control.c, adc.c, ...)
│   ├── Drivers/             # HAL 库 & CMSIS
│   └── MDK-ARM/             # Keil 工程文件
├── 交互/                    # STM32F103 交互节点
│   ├── Core/
│   │   ├── Inc/             # 头文件 (OLED.h, i2c.h, can.h, ...)
│   │   └── Src/             # 源文件 (main.c, OLED.c, i2c.c, ...)
│   ├── Drivers/             # HAL 库 & CMSIS
│   └── MDK-ARM/             # Keil 工程文件
└── README.md
```

## CAN 总线协议

| ID | 发送方 | 数据内容 | 长度 |
|----|--------|----------|------|
| 0x001 | 主控 | 报警信号 `"Warning"` | 7 字节 |
| 0x001 | 交互 | 测试数据 | 8 字节 |
| 0x002 | 检测 | 温度值 (float×100, 大端序) | 2 字节 |
