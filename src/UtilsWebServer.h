/**
 * @file UtilsWebServer.h
 * @brief Web 控制面板服务器
 *
 * 在 ESP32 上运行轻量级 HTTP 服务器，提供浏览器端的设备管理面板。
 * 功能包括：词库数据库浏览、JSON 导入导出、词库删除、学习统计查看、音量亮度调节。
 * 仅在 WiFi 连接成功后启动，前端页面从 SD 卡加载。
 */

#pragma once

#include "globals.h"

/**
 * 初始化 Web 服务器
 *
 * 注册所有 API 路由，启动 HTTP 服务器。
 * 仅在 WiFi 已连接时调用。启动后在串口输出访问地址。
 */
void initWebServer();

/**
 * Web 服务器循环处理
 *
 * 非阻塞地处理待处理的 HTTP 请求。在主 loop() 中每次迭代调用。
 */
void handleWebServer();
