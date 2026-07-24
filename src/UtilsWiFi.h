/**
 * @file UtilsWiFi.h
 * @brief WiFi 连接与时间同步工具
 *
 * 提供从 SD 卡读取 WiFi 凭据并自动连接的功能，
 * 以及通过 NTP 获取格式化时间字符串的功能。
 * WiFi 连接成功后会自动同步 NTP 时间（UTC+8）。
 */

#pragma once

#include "globals.h"

/**
 * 获取当前 NTP 时间的格式化字符串
 *
 * 返回格式为 "YY-MM-DD HH:MM"（如 "26-03-30 16:28"）的时间字符串。
 * 如果 WiFi 未连接或 NTP 时间未同步，则使用 millis() 的值作为回退，
 * 返回开机以来的毫秒数字符串。
 *
 * @return 格式化的时间字符串，或 millis() 毫秒数字符串
 */
String getNtpTimeString();

/**
 * 将 RSSI 信号强度转为可视指示符
 *
 * @param rssi 信号强度值（负数，越大越强）
 * @return 信号指示字符串
 */
String rssiIndicator(int rssi);

/**
 * 处理扫描结果，去重并按信号强度排序
 *
 * @param count WiFi.scanNetworks() 返回的网络数量
 */
void processWiFiScanResults(int count);

/**
 * 执行 WiFi 连接
 *
 * 使用选中的 SSID 和输入的密码尝试连接，超时 10 秒。
 * 连接成功后自动同步 NTP 时间、启用省电、保存凭据并启动 Web 控制台。
 * 连接失败时显示错误提示后返回列表。
 * 调用者负责处理成功后的页面跳转。
 */
void attemptWiFiConnect();

/**
 * 保存一组 WiFi 凭据到 SD 卡
 *
 * 若同名 SSID 已存在则更新密码，否则追加新条目。
 * 将完整列表序列化写回 /words_study/config.json。
 *
 * @param ssid 网络名称
 * @param pass 密码
 */
void saveWiFiCredential(const String &ssid, const String &pass);

/**
 * 查找已保存的 WiFi 密码
 *
 * @param ssid 要查找的网络名称
 * @return 已保存的密码，未找到返回空字符串
 */
String findSavedPassword(const String &ssid);
