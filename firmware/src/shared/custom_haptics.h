// Copyright 2026 ldafogel contributors. SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdint.h>

namespace custom_haptics {
// Additive extension, independently identified from upstream firmware version.
constexpr uint8_t capabilityRegister=0x30, configRegister=0x31, leaseRegister=0x32;
constexpr uint8_t size=19, maxDetents=10;
constexpr uint32_t leaseMs=500;
// [version, layer, count, stiffness/16, damping/256, maxPWM, deadband,
//  captureRadius (0=unlimited), slewPWM/tick, positions[10]]
// Positions/radii/deadband use 0..255 full travel. Unused positions are ignored.
inline bool valid(const uint8_t *p) {
  if(p[0]!=1 || p[1]>7 || p[2]>maxDetents || p[5]>254 || p[6]>32)return false;
  if(p[2] && (!p[3] || !p[5] || !p[8]))return false;
  for(uint8_t i=1;i<p[2];++i)if(p[9+i]<=p[8+i])return false;
  return true;
}
// Deterministic controller, independent of Arduino and calibration. Position
// and velocity use 1020 full-travel units and units/sec respectively.
class Spring {
 public:
  uint8_t config[size]={};
  bool active=false;
  void stop(){active=false;drive_=0;}
  bool apply(const uint8_t *p,uint32_t now) {
    if(!valid(p))return false;
    for(uint8_t i=0;i<size;++i)config[i]=p[i];
    active=p[2]!=0;lastLease_=now;drive_=0;nearest_=0;
    return true;
  }
  bool alive(uint32_t now) {
    if(active && uint32_t(now-lastLease_)>=leaseMs)stop();
    return active;
  }
  void renew(uint32_t now){if(alive(now))lastLease_=now;}
  int16_t tick(int16_t position,int32_t velocity,uint32_t now) {
    if(!alive(now))return 0;
    // Nearest detent, with two travel counts of hysteresis at boundaries.
    int16_t best=position-int16_t(config[9+nearest_])*4;
    if(best<0)best=-best;
    for(uint8_t i=0;i<config[2];++i) {
      int16_t distance=position-int16_t(config[9+i])*4;
      if(distance<0)distance=-distance;
      if(distance+8<best){nearest_=i;best=distance;}
    }
    if(config[7] && best>int16_t(config[7])*4){drive_=0;return 0;}
    int16_t error=int16_t(config[9+nearest_])*4-position;
    const int16_t deadband=int16_t(config[6])*4;
    if(error>deadband)error-=deadband;
    else if(error<-deadband)error+=deadband;
    else error=0;
    int32_t wanted=int32_t(error)*config[3]/16-int32_t(velocity)*config[4]/256;
    const int16_t limit=config[5];
    if(wanted>limit)wanted=limit;
    if(wanted<-limit)wanted=-limit;
    const int16_t step=config[8];
    if(wanted>drive_+step)drive_+=step;
    else if(wanted<drive_-step)drive_-=step;
    else drive_=wanted;
    // Never drive farther into a calibrated endpoint, including damping.
    if((position<=0 && drive_<0) || (position>=1020 && drive_>0))drive_=0;
    return int16_t(drive_);
  }
 private:
  int16_t drive_=0;
  uint32_t lastLease_=0;
  uint8_t nearest_=0;
};
}
