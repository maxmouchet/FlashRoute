/* Copyright (C) 2019 Neo Huang - All Rights Reserved */
#pragma once

#include <iostream>
#include <string>

#include "flashroute/prober.h"

namespace flashroute {

// 2^16 wrap-around interval for timestamp of UDP probe.
const uint32_t kTimestampSlot = 65536;

/**
 * UDP Prober handles packet construction and response parsing.
 *
 * Example:
 *
 * PacketReceiverCallback callback =
 *    [](uint32_t destination, uint32_t responder,
 *                    uint8_t distance, bool fromDestination) {
 *      // The tracerouting logic on response.
 *    };
 *
 * UdpProber prober(
 *    callback,   // Callback function to handle responses.
 *    0,          // Checksum offset to support discovery-optimized mode.
 *    1,          // 0 stands for preprobing, 1 stands for main probing.
 *    53,         // Destination port number
 *    "message payload");   //payload message.
 *
 * // Pass prober instance to network manager, so users can call
 * schedualProbeRemoteHost to issue probe or process responses in callback func.
 * NetworkManager networkManager(
 *  &prober,  // The prober to process packets.
 *  "eth0",   // The interface to send the probe.
 *  100000    // The packet sending rate.
 * );
 *
 */

class UdpProber : public virtual Prober {
 public:
  // Metrics
  uint64_t checksumMismatches;
  uint64_t distanceAbnormalities;

  UdpProber(PacketReceiverCallback* callback, const int32_t checksumOffset,
            const uint8_t probePhaseCode, const uint16_t destinationPort,
            const std::string& payloadMessage);

  // Construct probe.
  size_t packProbe(const uint32_t destinationIp, const uint32_t sourceIp,
                   const uint8_t ttl, uint8_t* packetBuffer) override;

  // Parse responses.
  void parseResponse(uint8_t* buffer, size_t size,
                     SocketType socketType) override;

  // Change checksum offset (support discovery-optimized mode.)
  void setChecksumOffset(int32_t checksumOffset);

  // Put here for testing purpose.
  uint16_t getChecksum(const uint16_t* ipaddress, uint16_t offset) const;

  // Put here for testing purpose.
  uint16_t getChecksum(const uint8_t protocolValue, size_t packetLength,
                       const uint16_t* src_addr, const uint16_t* dest_addr,
                       uint16_t* buff) const;

  // Put here for testing purpose.
  uint16_t getTimestamp() const;

 private:
  PacketReceiverCallback* callback_;
  int32_t checksumOffset_;
  uint8_t probePhaseCode_;
  uint16_t destinationPort_;
  std::string payloadMessage_;
};

}  // namespace flashroute