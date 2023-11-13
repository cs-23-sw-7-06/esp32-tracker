#include "wifi.hpp"

#include <stdio.h>

static void print_management_subtype(const FrameControl &frame_control) {
  const auto management_subttype = (ManagementSubType)frame_control.subtype;
  printf("Type: Management Subtype: ");
  switch (management_subttype) {
  case ManagementSubType::AssociationRequest:
    printf("Association Request\n");
    return;

  case ManagementSubType::AssociationResponse:
    printf("Association Response\n");
    return;

  case ManagementSubType::ReassociationRequest:
    printf("Reassociation Request\n");
    return;

  case ManagementSubType::ReassociationResponse:
    printf("Reassociation Response\n");
    return;

  case ManagementSubType::ProbeRequest:
    printf("Probe Request\n");
    return;

  case ManagementSubType::ProbeResponse:
    printf("Probe Response\n");
    return;

  case ManagementSubType::TimingAdvertisement:
    printf("Timing Advertisement\n");
    return;

  case ManagementSubType::Beacon:
    printf("Beacon\n");
    return;

  case ManagementSubType::ATIM:
    printf("ATIM\n");
    return;

  case ManagementSubType::Disassociation:
    printf("Disassociation\n");
    return;

  case ManagementSubType::Authentication:
    printf("Authentication\n");
    return;

  case ManagementSubType::Deauthentication:
    printf("Deauthentication\n");
    return;

  case ManagementSubType::Action:
    printf("Action\n");
    return;

  case ManagementSubType::ActionNoAck:
    printf("Action No Ack\n");
    return;
  }

  printf("Reserved\n");
}

static void print_control_subtype(const FrameControl &frame_control) {
  printf("Type: Control Subtype: ");

  switch (frame_control.subtype) {
  case 0b0000:
  case 0b0001:
  case 0b0010:
  case 0b1111:
    printf("Reserved");
    break;

  case 0b0011:
    printf("TACK");
    break;

  case 0b0100:
    printf("Beamforming Report Poll");
    break;

  case 0b0101:
    printf("VHT NDP Announcement");
    break;

  case 0b0110:
    printf("Control Frame Extension");
    break;

  case 0b0111:
    printf("Control Wrapper");
    break;

  case 0b1000:
    printf("Block Ack Request");
    break;

  case 0b1001:
    printf("Block Ack");
    break;

  case 0b1010:
    printf("PS-Poll");
    break;

  case 0b1011:
    printf("RTS");
    break;

  case 0b1100:
    printf("CTS");
    break;

  case 0b1101:
    printf("Ack");
    break;

  case 0b1110:
    printf("CF-End");
    break;
  }

  printf("\n");
}

void print_data_subtype(const FrameControl &frame_control) {
  const auto data_subtype = (DataSubType)frame_control.subtype;
  printf("Type: Data Subtype: ");
  switch (data_subtype) {
  case DataSubType::Data:
    printf("Data\n");
    return;

  case DataSubType::Null:
    printf("Null\n");
    return;

  case DataSubType::QoSData:
    printf("QoS Data\n");
    return;

  case DataSubType::QoSDataWithCFAck:
    printf("QoS Data +CF-Ack\n");
    return;

  case DataSubType::QoSDataWithCFPoll:
    printf("QoS Data +CF-Poll\n");
    return;

  case DataSubType::QoSDataWithCFAckAndCFPoll:
    printf("QoS Data +CF-Ack +CF-Poll\n");
    return;

  case DataSubType::QoSNull:
    printf("QoS Null\n");
    return;

  case DataSubType::QoSCFPoll:
    printf("QoS CF-Poll\n");
    return;

  case DataSubType::QoSCFAckWithCFPoll:
    printf("QoS CF-Ack +CF-Poll\n");
    return;
  }

  printf("Reserved\n");
}

static void print_extension_subtype(const FrameControl &frame_control) {
  printf("Type: Extension Subtype: ");

  switch (frame_control.subtype) {
  case 0b0000:
    printf("DMG Beacon");
    break;

  case 0b0001:
    printf("SIG Beacon");
    break;

  default:
    printf("Reserved");
    break;
  }

  printf("\n");
}

void print_frame_control_type(const FrameControl &frame_control) {
  switch (frame_control.type) {
  case FrameControlType::Management:
    print_management_subtype(frame_control);
    break;
  case FrameControlType::Control:
    print_control_subtype(frame_control);
    break;
  case FrameControlType::Data:
    print_data_subtype(frame_control);
    break;
  case FrameControlType::Extension:
    print_extension_subtype(frame_control);
    break;
  }
}

void print_mac_address(const MacAddress &address) {
  printf("%02x:%02x:%02x:%02x:%02x:%02x\n", address.bytes[0], address.bytes[1],
         address.bytes[2], address.bytes[3], address.bytes[4],
         address.bytes[5]);
}
