#include <initializer_list>

#include "shared/custom_haptics.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
using custom_haptics::Spring;
static uint8_t profile[19]={1,0,1,32,48,110,1,0,2,51};
static int settle(Spring &s,int pos,int velocity=0,uint32_t now=0) {
 int drive=0;for(int i=0;i<200;++i)drive=s.tick(pos,velocity,now);return drive;
}
// An illustrative plant only: does not substitute for a real unit's friction,
// power supply, loading or measured loop cadence.
static void returnModel(int start) {
 Spring s;assert(s.apply(profile,0));double x=start,v=0;
 for(unsigned tick=0;tick<6000;++tick) {
  const uint32_t ms=tick/2;if(tick%200==0)s.renew(ms);
  const double drive=s.tick(int16_t(x),int32_t(v),ms);
  if(v==0 && drive<60 && drive>-60)continue;
  const double direction=v>0?1:v<0?-1:drive>0?1:-1;
  const double next=v+(drive-direction*40-v/20)*100*.0005;
  if(v*next<0 && drive<60 && drive>-60)v=0;else v=next;
  x+=v*.0005;
  if(x<0){x=0;v=0;}if(x>1020){x=1020;v=0;}
 }
 assert(x>156 && x<240); // above 15% rearm, near the 20% resting point
}
int main() {
 returnModel(0);returnModel(1020);
 Spring s;assert(!s.active && s.tick(0,0,0)==0);
 assert(s.apply(profile,0));assert(settle(s,0)>0);assert(settle(s,1020)<0);
 assert(settle(s,204)==0);assert(settle(s,206)==0); // smooth dead zone
 assert(settle(s,100)==110 && settle(s,400)==-110); // independent force cap
 assert(s.apply(profile,0));assert(s.tick(0,0,0)==2 && s.tick(0,0,0)==4); // bounded onset
 assert(s.apply(profile,0));const int still=settle(s,164);
 assert(s.apply(profile,0));assert(settle(s,164,1000)<still); // damping opposes return velocity
 assert(s.apply(profile,0));assert(settle(s,164,-1000)>still); // resisting a pull
 assert(s.apply(profile,0));assert(settle(s,0,100000)==0); // no outward endpoint drive
 assert(s.apply(profile,0));assert(s.alive(499));s.renew(499);assert(s.alive(998));
 assert(!s.alive(999));s.renew(1000);assert(!s.active && s.tick(0,0,1000)==0); // no resurrection
 assert(s.apply(profile,0xffffff00UL));assert(s.alive(243));assert(!s.alive(244));
 uint8_t p[19];memcpy(p,profile,19);p[2]=0;assert(s.apply(p,0) && !s.active);
 memcpy(p,profile,19);p[2]=11;assert(!s.apply(p,0));
 p[2]=2;p[10]=51;assert(!s.apply(p,0));p[10]=50;assert(!s.apply(p,0));
 p[10]=200;assert(s.apply(p,0));assert(settle(s,850)<0);assert(settle(s,740)>0);
 memcpy(p,profile,19);p[7]=10;assert(s.apply(p,0));assert(settle(s,0)==0 && settle(s,180)>0);
 for(unsigned index: {0u,1u,3u,5u,6u,8u}) {
  memcpy(p,profile,19);p[index]=index==0?2:index==1?8:index==5?255:index==6?33:0;
  assert(!s.apply(p,0));
 }
 // All possible 8-bit positions: bounded drive and coherent sign at rest.
 for(int pos=0;pos<=1020;++pos) {
  assert(s.apply(profile,0));int drive=settle(s,pos);
  assert(drive>=-110 && drive<=110);
  assert(pos<200?drive>0:pos>208?drive<0:drive==0);
 }
 // Five arbitrary detents, including endpoints; crossing a boundary selects
 // the new nearest one while small boundary noise retains its previous choice.
 memcpy(p,profile,19);p[2]=5;p[9]=0;p[10]=64;p[11]=128;p[12]=191;p[13]=255;
 assert(s.apply(p,0));assert(settle(s,100)<0);assert(settle(s,136)>0);
 assert(settle(s,127)>0);assert(settle(s,120)<0);
 puts("PASS: spring force, damping, detents, slew, endpoints, packet validation and lease expiry/wrap");
}
