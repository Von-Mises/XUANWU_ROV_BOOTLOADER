# 玄武号ROV BootLoader程序

## 写在前面

![image-20230601085820607](C:\Users\Laptop\AppData\Roaming\Typora\typora-user-images\image-20230601085820607.png)

​                                                                                 玄武号ROV

本程序是玄武号ROV的BootLoader程序，主控芯片采用STM32H743IIT6，基于嵌入式轻量级TCP/IP协议栈Lwip RAW编程接口实现。

下位机作为TCP以及TFTP服务器，上位机作为客户端。下位机监听来自上位机目标端口号‘5678’的TCP连接请求与目标端口号‘69’的UDP连接请求。

TCP连接负责向上位机回显IAP过程的倒计时，基于UDP实现的TFTP负责接收上位机下发的新APP程序(bin文件)并向板载Flash烧录。  

操作流程见Doc目录下的说明文档，所需工具在Tools目录。

## 软件环境

|    Toolchain/IDE     |       MDK-ARM V5       |
| :------------------: | :--------------------: |
|      ARM::CMSIS      |         5.4.0          |
|    ARM::CMSIS-DSP    |         1.14.3         |
|  ARM::CMSIS-Driver   |         2.7.2          |
|  Keil::ARM_Compiler  |         1.6.0          |
| Keil::MDK-Middleware |         7.7.0          |
| Keil::STM32H7xx_DFP  |         3.0.0          |
|      CMSIS-RTOS      |          1.02          |
|     STM32CubeMx      |         5.3.0          |
|         Lwip         |         2.0.3          |
|       Package        | STM32Cube FW_H7 V1.5.0 |
