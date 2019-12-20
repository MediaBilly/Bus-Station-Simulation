#include <string.h>
#include "../headers/shared_segment.h"

void Shared_Segment_Init(Shared_segment *segment) {
  memset(segment,0,sizeof(Shared_segment));
}