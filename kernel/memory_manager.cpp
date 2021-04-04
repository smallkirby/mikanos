#include<sys/types.h>
#include"memory_manager.hpp"

BitmapMemoryManager::BitmapMemoryManager()
  : alloc_map_{}, range_begin_{FrameID{0}}, range_end_{FrameID{kFrameCount}} 
{
}

WithError<FrameID> BitmapMemoryManager::Allocate(size_t num_frames)
{
  size_t start_frame_id = range_begin_.ID();
  while(1==1){
    size_t ix=0;
    for(; ix!=num_frames; ++ix){
      if(start_frame_id + ix >= range_end_.ID()){
        return {kNullFrame, MAKE_ERROR(Error::kNoEnoughMemory)};
      }
      if(GetBit(FrameID{start_frame_id + ix})){
        break;
      }
    }

    if(ix == num_frames){
      MarkAllocated(FrameID{start_frame_id}, num_frames);
      return {FrameID{start_frame_id}, MAKE_ERROR(Error::kSuccess)};
    }
    start_frame_id += ix + 1;
  }
}

Error BitmapMemoryManager::Free(FrameID start_frame, size_t num_frames)
{
  for(size_t ix=0; ix!=num_frames; ++ix){
    SetBit(FrameID{start_frame.ID() + ix}, false);
  }
  return MAKE_ERROR(Error::kSuccess);
}

void BitmapMemoryManager::MarkAllocated(FrameID start_frame, size_t num_frames)
{
  for(size_t ix=0; ix!=num_frames; ++ix){
    SetBit(FrameID{start_frame.ID() + ix}, true);
  }
}

void BitmapMemoryManager::SetMemoryRange(FrameID range_begin, FrameID range_end)
{
  range_begin_ = range_begin;
  range_end_ = range_end;
}

bool BitmapMemoryManager::GetBit(FrameID frame) const
{
  auto line_index = frame.ID() / kBitsPerMapLine;
  auto bits_index = frame.ID() % kBitsPerMapLine;
  return (alloc_map_[line_index] & (static_cast<MapLineType>(1) << bits_index)) != 0;
}

void BitmapMemoryManager::SetBit(FrameID frame, bool allocated) 
{
  auto line_index = frame.ID() / kBitsPerMapLine;
  auto bits_index = frame.ID() % kBitsPerMapLine;
  if(allocated){
    alloc_map_[line_index] |= (static_cast<MapLineType>(1) << bits_index);
  }else{
    alloc_map_[line_index] &= ~(static_cast<MapLineType>(1) << bits_index);
  }
}

extern "C" caddr_t program_break, program_break_end;

Error InitializeHeap(BitmapMemoryManager &memory_manager)
{
  const int kHeapFrames = 64 * 512;
  const auto heap_start = memory_manager.Allocate(kHeapFrames);
  if(heap_start.error){
    return heap_start.error;
  }
  program_break = reinterpret_cast<caddr_t>(heap_start.value.Frame());
  program_break_end = program_break + kHeapFrames * kBytesPerFrame;
  return MAKE_ERROR(Error::kSuccess);
}