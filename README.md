# STM32F4 IAP Bootloader




> 基于 UART 的 STM32F4 系列 bootloader，支持 EasyLogger 日志、CRC32 校验、Flash 编程、固件升级

[English](./README_EN.md) | [中文](./README.md)
作者：[古译汉书-CSDN博客](https://blog.csdn.net/m0_56408226?type=blog)

## 目录

- [特性](#-特性)
- [快速开始](#-快速开始)
- [项目结构](#-项目结构)
- [通信协议](#-通信协议)
- [使用方法](#-使用方法)
- [API 参考](#-api-参考)
- [技术细节](#-技术细节)
- [常见问题](#-常见问题)
- [贡献指南](#-贡献指南)
- [个人CSDN博客](#-个人CSDN博客)
- [联系方式](#-联系方式)

---

## 特性

### 核心功能

- **UART 升级** - 基于串口的 IAP (In-Application Programming) 引导程序
- **安全校验** - CRC16 数据包校验 + CRC32 固件完整性验证
- **Magic Header** - 应用固件合法性验证机制
- **Flash 编程** - 支持 STM32F4xx 系列内部 Flash 擦除与写入
-  **多种启动模式** - 按键启动 / 串口触发启动 / 自动启动

### 日志系统

- **EasyLogger** - 高性能嵌入式日志库
- **多级别日志** - ASSERT / ERROR / WARN / INFO / DEBUG / VERBOSE
- **格式化输出** - 支持时间戳、标签、日志级别格式化

### 驱动支持

- **按键检测** - 按键长按进入 bootloader 模式
- **LED 指示** - 运行状态可视化指示
- **延时系统** - 定时器延时驱动
- **环形缓冲区** - 高效 UART 数据接收

---

## 快速开始

### 硬件要求

| 组件  | 规格                      |
| ----- | ------------------------- |
| MCU   | STM32F407VG / STM32F407xx |
| Flash | 512KB (最大 1MB)          |
| SRAM  | 192KB                     |
| 通信  | UART3 (115200 8N1)        |
| 按键  | 1个 (PA0)                 |
| LED   | 1个 (PB0)                 |

### 引脚配置

```
UART3:
  - TX: PC10
  - RX: PC11

按键: PA0 (低电平按下)
LED:  PB0 (低电平点亮)
```

### 编译步骤

1. **克隆项目**

```bash
git clone https://github.com/xiao-wang-lll/stm32-iap-bootloader.git
```

2. **使用 Keil MDK 打开项目**

```bash
# 打开 mdk/stm32F407.uvprojx
```

3. **配置工程选项**

```
Target -> Xtal (MHz): 8
C/C++ -> Define: USE_STDPERIPH_DRIVER, STM32F40_41xxx
```

4. **编译并下载**

```bash
# Keil MDK: F7 编译, F8 下载
```

### 启动流程

```
上电/复位
    │
    ▼
┌─────────────────────┐
│   Bootloader 启动   │  (0x08000000, 48KB)
│   初始化外设         │
│   EasyLogger 启动   │
└──────────┬──────────┘
           │
           ▼
    ┌──────────────┐
    │ 按键检测 (3s) │
    └──────┬───────┘
           │
    ┌──────┴───────┐
    │   按下?      │
    └──────┬───────┘
      是   │   否
      ▼    │
 ┌─────────┐  │
 │ 进入BL  │  ▼
 │  模式   │  检查 Magic Header
 └─────────┘        │
                     ▼
              ┌──────────────┐
              │ Magic Header │
              │   有效?      │
              └──────┬───────┘
                否   │   是
                ▼    │
         ┌─────────┐  │
         │ 进入BL  │  ▼
         │  模式   │  检查 CRC32
         └─────────┘        │
                     ┌───────┴───────┐
                     │   CRC 校验    │
                     │    通过?      │
                     └───────┬───────┘
                       否   │   是
                       ▼    │
                  ┌──────── │ 进入BL ─┐  │
                  │  ▼
                  │  模式   │  跳转 App
                  └─────────┘    │
                                  ▼
                          ┌─────────────┐
                          │   启动 App  │  (0x08010000)
                          └─────────────┘
```

---

## 项目结构

```
stm32F4_tomain_usart_easylogger/
├── app/                          # 应用层代码
│   ├── main.c                    # 主程序入口
│   ├── bootloader.c              # Bootloader 核心逻辑
│   ├── board.c                   # 板级初始化
│   ├── board.h                   # 板级头文件
│   ├── jumpapp.s                 # 跳转汇编代码
│   ├── magic_header.c            # Magic Header 管理
│   └── magic_header.h            # Magic Header 头文件
│
├── driver/                       # 驱动层
│   ├── bl_usart/                 # UART 驱动
│   ├── console/                  # 控制台驱动
│   ├── key/                      # 按键驱动
│   ├── led/                      # LED 驱动
│   ├── stm32_flash/              # Flash 驱动
│   └── tim_delay/                # 定时器延时驱动
│
├── third_lib/                    # 第三方库
│   ├── EasyLogger-master/        # EasyLogger 日志库
│   ├── freertos/                 # FreeRTOS (预留)
│   ├── crc/                      # CRC 校验库
│   ├── ringbuffer/               # 环形缓冲区
│   └── utils/                    # 工具函数
│
├── firmware/                     # 固件相关
│   ├── cmsis/                    # CMSIS 头文件
│   └── driver/                   # STM32 HAL 驱动
│
├── mdk/                          # Keil 工程目录
│   └── stm32F407.uvprojx         # Keil 工程文件
│
└── README.md                     # 本文件
```

---

## ? 通信协议

### 数据包格式

| 字段    | 长度   | 说明              |
| ------- | ------ | ----------------- |
| Header  | 1 字节 | 0xAA (固定)       |
| Opcode  | 1 字节 | 操作码            |
| Length  | 2 字节 | 负载长度 (小端)   |
| Payload | N 字节 | 负载数据          |
| CRC16   | 2 字节 | CRC16 校验 (小端) |

### 操作码定义

| Opcode | 名称    | 说明            |
| ------ | ------- | --------------- |
| 0x01   | INQUERY | 查询 (版本/MTU) |
| 0x81   | ERASE   | Flash 擦除      |
| 0x82   | PROGRAM | Flash 编程      |
| 0x83   | VERIFY  | CRC32 校验      |
| 0x21   | RESET   | 系统复位        |
| 0x22   | BOOT    | 启动应用程序    |

### 响应格式

| 字段    | 长度   | 说明              |
| ------- | ------ | ----------------- |
| Header  | 1 字节 | 0x55 (固定)       |
| Opcode  | 1 字节 | 操作码            |
| ErrCode | 1 字节 | 错误码            |
| Length  | 2 字节 | 负载长度 (小端)   |
| Payload | N 字节 | 负载数据          |
| CRC16   | 2 字节 | CRC16 校验 (小端) |

### 错误码

| ErrCode | 说明       |
| ------- | ---------- |
| 0x00    | 成功       |
| 0x01    | 操作码错误 |
| 0x02    | 数据溢出   |
| 0x03    | 超时       |
| 0x04    | 格式错误   |
| 0x05    | 校验失败   |
| 0x06    | 参数错误   |
| 0xFF    | 未知错误   |

---

## 使用方法

### 1. 启动 Bootloader 模式

**方式一: 按键触发**

上电时按住按键超过 3 秒，将进入 bootloader 模式。

**方式二: 串口触发**

上电后 3 秒内通过串口发送任意数据，将进入 bootloader 模式。

**方式三: Magic Header 无效**

如果应用程序的 Magic Header 无效或 CRC32 校验失败，将自动进入 bootloader 模式。

### 2. 固件升级流程

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   主机 PC   │────?│  UART 通信  │────?│  STM32 BL   │
└─────────────┘     └─────────────┘     └─────────────┘
       │                                       │
       │  1. 查询版本/MTU                       │
       │?──────────────────────────────────────┤
       │                                        │
       │  2. 擦除 Flash                          │
       │───────────────────────────────────────?│
       │                                        │
       │  3. 写入固件数据                        │
       │───────────────────────────────────────?│
       │                                        │
       │  4. CRC32 校验                          │
       │───────────────────────────────────────?│
       │                                        │
       │  5. 启动应用                            │
       │───────────────────────────────────────?│
```

### 3. 串口命令示例

```python
# Python 示例: 使用串口升级固件
import serial
import struct
import crcmod

class STM32Bootloader:
    def __init__(self, port, baudrate=115200):
        self.serial = serial.Serial(port, baudrate, timeout=1)
        
    def send_packet(self, opcode, payload=b''):
        # 构建数据包
        packet = struct.pack('BBH', 0xAA, opcode, len(payload)) + payload
        crc = crcmod.Crc16Ccitt().calc(packet)
        packet += struct.pack('<H', crc)
        self.serial.write(packet)
        
    def read_response(self):
        # 读取响应
        header = self.serial.read(1)
        if header != b'\x55':
            return None
        opcode = self.serial.read(1)
        errcode = self.serial.read(1)
        length = struct.unpack('<H', self.serial.read(2))[0]
        payload = self.serial.read(length)
        crc = struct.unpack('<H', self.serial.read(2))[0]
        return {'opcode': opcode, 'errcode': errcode, 'payload': payload}
    
    def get_version(self):
        self.send_packet(0x01, b'\x00')  # 查询版本
        return self.read_response()
    
    def erase_flash(self, address, size):
        self.send_packet(0x81, struct.pack('<II', address, size))
        return self.read_response()
    
    def program_flash(self, address, data):
        payload = struct.pack('<II', address, len(data)) + data
        self.send_packet(0x82, payload)
        return self.read_response()
    
    def verify_firmware(self, address, size, crc32):
        payload = struct.pack('<III', address, size, crc32)
        self.send_packet(0x83, payload)
        return self.read_response()
    
    def boot_app(self):
        self.send_packet(0x22, b'')
        return self.read_response()
```

---

## API 参考

### Bootloader 核心函数

```c
// 启动 bootloader
int bootloader_main(void);

// 应用验证
bool application_validate(void);

// 启动应用程序
void boot_application(void);
```

### Flash 操作

```c
// 解锁 Flash
void stm32_flash_unlock(void);

// 锁定 Flash
void stm32_flash_lock(void);

// 擦除 Flash
void stm32_flash_erase(uint32_t address, uint32_t size);

// 编程 Flash
void stm32_flash_program(uint32_t address, uint8_t *data, uint32_t size);
```

### Magic Header

```c

typedef struct
{
    uint32_t magic;         // 魔数，用于标识这是一个有效的魔术头
    uint32_t bitmask;       // 位掩码，用于标识哪些字段有效
    uint32_t reserved1[6];  // 保留字段，供将来扩展使用

    uint32_t data_type;     // 类型，根据type类型选择固件下载位置,比如下载到eeprom或者w25Q64或者MCU的app区
    uint32_t data_offset;   // 固件文件相对于bin文件里面的magic_header的偏移
    uint32_t data_address;  // 固件写入的实际地址
    uint32_t data_length;   // 固件长度
    uint32_t data_crc32;    // 固件的CRC32校验值
    uint32_t reserved2[11]; // 保留字段，供将来扩展使用

    char version[128];      // 固件版本字符串

    uint32_t reserved3[6];  // 保留字段，供将来扩展使用
    uint32_t this_address;  // 该结构体在存储介质中的实际地址
    uint32_t this_crc32;    // 该结构体本身的CRC32校验值
} magic_header_t;

// 验证 Magic Header
bool magic_header_validate(void);

// 获取应用地址
uint32_t magic_header_get_address(void);

// 获取固件长度
uint32_t magic_header_get_length(void);

// 获取 CRC32
uint32_t magic_header_get_crc32(void);
```

---

## 技术细节

### 内存布局

```
Flash 分布 (512KB):
┌────────────────────┐ 0x08000000
│   Bootloader       │ 48KB
│   (0x08000000)     │
├────────────────────┤ 0x0800C000
│   Application      │ ~448KB
│   (0x08010000)     │
└────────────────────┘ 0x08080000

SRAM 分布 (192KB):
┌────────────────────┐ 0x20000000
│   Static Data      │
├────────────────────┤ 
│   Heap             │
├────────────────────┤ 
│   Stack            │
└────────────────────┘ 0x20030000
```

### CRC 校验

- **CRC16**: 用于数据包校验 (传输层)
- **CRC32**: 用于固件完整性验证 (应用层)

### 状态机

Bootloader 使用有限状态机 (FSM) 解析串口数据包:

```
┌───────────────┐
│    HEADER     │ ?──────┐
└───────┬───────┘        │
        │ 0xAA           │
        ▼                │
┌───────────────┐        │
│    OPCODE     │        │
└───────┬───────┘        │
        │ Valid           │
        ▼                │
┌───────────────┐        │
│    LENGTH     │        │
└───────┬───────┘        │
        │ Valid           │
        ▼                │
┌───────────────┐        │
│    PAYLOAD    │        │
└───────┬───────┘        │
        │ Complete        │
        ▼                │
┌───────────────┐        │
│    CRC16      │────────┘
└───────────────┘
```

---

## 常见问题

### Q1: 如何修改 Bootloader 大小?

修改 `app/bootloader.c` 中的配置:

```c
#define BL_SIZE (48 * 1024)  // 修改为需要的大小
```

### Q2: 如何修改应用程序起始地址?

1. 修改 Bootloader 配置:

```c
#define APP_VTOR_ADDR 0x08010000
```

2. 修改应用程序的链接脚本，将起始地址改为 `0x08010000`

### Q3: 如何关闭串口触发启动?

修改 `app/bootloader.c`:

```c
// 注释掉此行
// #define third_wait_boot 1
```

### Q4: 升级失败怎么办?

1. **检查串口连接**: 确认波特率为 115200
2. **检查 CRC 校验**: 确认固件 CRC32 正确
3. **重新上电**: 重新进入 bootloader 模式

---

## 贡献指南

欢迎提交 Issue 和 Pull Request！

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add some amazing feature'`)
4. 推送分支 (`git push origin feature/amazing-feature`)
5. 打开 Pull Request

---
## 个人CSDN博客

https://blog.csdn.net/m0_56408226?type=blog

---

## 联系方式

17735813721@163.com
---


</p>

