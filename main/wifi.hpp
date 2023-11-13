#pragma once

#include <stdint.h>

struct MacAddress {
  uint8_t bytes[6];
} __attribute((packed));

static_assert(sizeof(MacAddress) == 6);

enum class FrameControlType : uint8_t {
  Management = 0b00,
  Control = 0b01,
  Data = 0b10,
  Extension = 0b11,
};

struct FrameControl {
  uint8_t protocol_version : 2;
  FrameControlType type : 2;
  uint8_t subtype : 4;
  uint8_t flags;
} __attribute((packed));

static_assert(sizeof(FrameControl) == 2);

enum class ManagementSubType : uint8_t {
  AssociationRequest = 0b0000,
  AssociationResponse = 0b0001,
  ReassociationRequest = 0b0010,
  ReassociationResponse = 0b0011,
  ProbeRequest = 0b0100,
  ProbeResponse = 0b0101,
  TimingAdvertisement = 0b0110,
  Beacon = 0b1000,
  ATIM = 0b1001,
  Disassociation = 0b1010,
  Authentication = 0b1011,
  Deauthentication = 0b1100,
  Action = 0b1101,
  ActionNoAck = 0b1110,
};

struct ManagementFrame {
  FrameControl frame_control;
  uint16_t duration;
  MacAddress destination_address;
  MacAddress transmitter_address;
} __attribute((packed));

enum class DataSubType {
  Data = 0b0000,
  Null = 0b0100,
  QoSData = 0b1000,
  QoSDataWithCFAck = 0b1001,
  QoSDataWithCFPoll = 0b1010,
  QoSDataWithCFAckAndCFPoll = 0b1011,
  QoSNull = 0b1100,
  QoSCFPoll = 0b1110,
  QoSCFAckWithCFPoll = 0b1111,
};

struct DataFrame {
  FrameControl frame_control;
  uint16_t duration;
  MacAddress receiver_address;
  MacAddress transmitter_address;
  MacAddress destination_address;
  MacAddress source_address;

  bool toAP() const {
    auto masked_bits = frame_control.flags & 0b00000011;
    return masked_bits == 0b00000001;
  }
} __attribute((packed));

void print_mac_address(const MacAddress &address);

void print_frame_control_type(const FrameControl &frame_control);

void print_data_subtype(const FrameControl &frame_control);

constexpr MacAddress create_mac_address(uint8_t b1, uint8_t b2, uint8_t b3,
                                        uint8_t b4, uint8_t b5, uint8_t b6) {
  MacAddress address;
  address.bytes[0] = b1;
  address.bytes[1] = b2;
  address.bytes[2] = b3;
  address.bytes[3] = b4;
  address.bytes[4] = b5;
  address.bytes[5] = b6;

  return address;
}
