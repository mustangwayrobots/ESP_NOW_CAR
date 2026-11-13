/**
 * ==============================================================================
 * Protocol.h - The radio "language" spoken by the car and the controller
 * ==============================================================================
 * THIS FILE MUST BE IDENTICAL in the ESP_NOW_CAR and ESP_NOW_Controller repos.
 * If you change anything in it: raise PROTOCOL_VERSION, copy the file to the
 * other repo, and flash BOTH the car and the controller.
 *
 * Every packet starts with PROTOCOL_VERSION. A packet with a different
 * version or a different size is ignored, so a car and a controller with
 * mismatched firmware can never drive each other by accident.
 *
 * Pairing (always on BINDING_CHANNEL):
 *   controller --BIND_REQUEST (broadcast, carries the drive channel)--> car
 *   controller <--BIND_ACK (sent back to that controller)-------------- car
 *   controller --BIND_CONFIRM (to that car, carries the drive channel)--> car
 *   The controller saves the car's MAC. The car saves the controller's MAC
 *   and the drive channel. Both restart into Normal Mode.
 *
 * Driving (on the saved drive channel):
 *   controller --DRIVE every 20-100 ms----> car
 *   controller <--TELEMETRY every 200 ms--- car  (only while DRIVE packets arrive)
 *   The controller shows CONNECTED only while TELEMETRY keeps arriving,
 *   so CONNECTED means "the car is really listening to this controller".
 * ==============================================================================
 */
#pragma once
#include <stdint.h>

// Version 1 was the old 25-byte packet with no version byte.
const uint8_t PROTOCOL_VERSION = 2;

enum PacketType : uint8_t {
  PKT_BIND_REQUEST = 0,  // controller -> everyone: "I want to pair"
  PKT_DRIVE        = 1,  // controller -> car: motor and weapon values
  PKT_BIND_ACK     = 2,  // car -> controller: "I heard you"
  PKT_BIND_CONFIRM = 3,  // controller -> car: "I heard you too, save me"
  PKT_TELEMETRY    = 4   // car -> controller: battery voltage (also proves the car is listening)
};

const uint8_t       BINDING_CHANNEL     = 1;    // pairing always happens on this Wi-Fi channel
const unsigned long TELEMETRY_PERIOD_MS = 200;  // the car replies this often while being driven

struct __attribute__((packed)) RC_Packet_Structure {
  uint8_t protocolVersion;  // always PROTOCOL_VERSION
  uint8_t packetType;       // one of the PacketType values above
  uint8_t channel;          // BIND packets: the drive channel (1, 6 or 11). Otherwise 0.
  int motor_A1; int motor_A2;   // DRIVE: 0-255
  int motor_B1; int motor_B2;   // DRIVE: 0-255
  int weapon;                   // DRIVE: servo angle 0-180
  int battery;                  // TELEMETRY: car battery in millivolts
};

// True if the received bytes are a packet in THIS protocol version
inline bool isCurrentProtocol(const uint8_t *data, int len) {
  return len == (int)sizeof(RC_Packet_Structure) && data[0] == PROTOCOL_VERSION;
}

// Sets the first three bytes of a packet. The other fields are left alone.
inline void setPacketHeader(RC_Packet_Structure &packet, uint8_t type, uint8_t channel = 0) {
  packet.protocolVersion = PROTOCOL_VERSION;
  packet.packetType      = type;
  packet.channel         = channel;
}

// The three Wi-Fi channels used for driving
inline bool isDriveChannel(uint8_t ch) {
  return ch == 1 || ch == 6 || ch == 11;
}
