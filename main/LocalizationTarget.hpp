#pragma once

#include "wifi.hpp"

enum class SnifferType {
  WiFi,
  Bluetooth,
};

struct LocalizationTarget {
  MacAddress m_mac_address;
  SnifferType m_sniffer_type;
  int m_rssi;

  LocalizationTarget(MacAddress mac_address, SnifferType sniffer_type, int rssi)
      : m_mac_address(mac_address), m_sniffer_type(sniffer_type), m_rssi(rssi) {
  }
};
