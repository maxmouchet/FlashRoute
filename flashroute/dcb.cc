#include <memory>
#include <mutex>

#include "flashroute/dcb.h"

namespace flashroute {

DestinationControlBlock::DestinationControlBlock(uint32_t ip,
                                                 uint32_t nextElement,
                                                 uint32_t previousElement,
                                                 uint8_t initialTtl)
    : ipAddress(ip),
      nextElementOffset(nextElement),
      previousElementOffset(previousElement),
      removed(false),
      initialBackwardProbingTtl(initialTtl),
      nextBackwardHop_(initialTtl),
      preprobedMark_(false),
      accurateDistanceMark_(false),
      nextForwardHop_(initialTtl + 1),
      forwardHorizon_(initialTtl) {
  visitMutex_ = std::make_unique<std::recursive_mutex>();
}

bool DestinationControlBlock::updateSplitTtl(uint8_t ttlToUpdate,
                                               bool confirmResult) {
  std::lock_guard<std::recursive_mutex> guard(*visitMutex_.get());
  bool result = !preprobedMark_;
  // If the target does not have any confirmed hop-distance, we are allowed to
  // update it.
  if (!accurateDistanceMark_) {
    {
      std::lock_guard<std::recursive_mutex> guard(
          *visitMutex_.get());  // ttlToProbeMutex_
      nextBackwardHop_ = ttlToUpdate;
      // update the initial TTL for backward probing.
      initialBackwardProbingTtl = ttlToUpdate;
      // Also update the next forward hop.
      nextForwardHop_ = ttlToUpdate + 1;
      forwardHorizon_ = ttlToUpdate;
      // If the updated TTL is from an accurate preprobing result, we lock the
      // future update.
      if (confirmResult) {
        accurateDistanceMark_ = true;
      }
      preprobedMark_ = true;
    }
  }
  return result;
}

uint8_t DestinationControlBlock::stopBackwardProbing() {
  std::lock_guard<std::recursive_mutex> guard(*visitMutex_.get());
  uint8_t remains = nextBackwardHop_;
  nextBackwardHop_ = 0;
  return remains;
}

uint8_t DestinationControlBlock::pullBackwardTask() {
  std::lock_guard<std::recursive_mutex> guard(*visitMutex_.get());
  if (nextBackwardHop_ > 0) {
    return nextBackwardHop_--;
  } else {
    return 0;
  }
}

bool DestinationControlBlock::hasBackwardTask() {
  std::lock_guard<std::recursive_mutex> guard(*visitMutex_.get());
  return nextBackwardHop_ > 0;
}

uint8_t DestinationControlBlock::peekBackwardTask() {
  std::lock_guard<std::recursive_mutex> guard(*visitMutex_.get());
  return nextBackwardHop_;
}

bool DestinationControlBlock::hasForwardTask() {
  std::lock_guard<std::recursive_mutex> guard(*visitMutex_.get());
  return forwardHorizon_ >= nextForwardHop_;
}

uint8_t DestinationControlBlock::pullForwardTask() {
  std::lock_guard<std::recursive_mutex> guard(*visitMutex_.get());
  if (forwardHorizon_ >= nextForwardHop_) {
    return nextForwardHop_++;
  } else {
      return 0;
  }
}

void DestinationControlBlock::stopForwardProbing() {
  std::lock_guard<std::recursive_mutex> guard(*visitMutex_.get());
  forwardHorizon_ = 0;
}

int16_t DestinationControlBlock::getMaxProbedDistance() {
  std::lock_guard<std::recursive_mutex> guard(*visitMutex_.get());
  return nextForwardHop_ - 1;
}

void DestinationControlBlock::setForwardHorizon(uint8_t forwardExploredHop) {
  std::lock_guard<std::recursive_mutex> guard(*visitMutex_.get());
  // forwardHorizon_ == 0 means that the forward probing is done;
  // therefore, we will not update the variable regarding the forward probing.
  if (forwardHorizon_ == 0) return;
  if (forwardExploredHop > forwardHorizon_) {
    forwardHorizon_ = forwardExploredHop;
  }
}

void DestinationControlBlock::resetProbingProgress(uint8_t ttl) {
  nextBackwardHop_ = ttl;
  initialBackwardProbingTtl = ttl;
  nextForwardHop_ = ttl + 1;
  forwardHorizon_ = ttl;
  removed = false;
}

}  // namespace flashroute


