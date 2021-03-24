#include<cstdint>
#include<cstddef>

#include"frame_buffer_config.hpp"

#define noreturn

void noreturn hlt(){
  while(1==1){
    __asm__("hlt");
  }
}

struct PixelColor{
  uint8_t r,g,b;
};

class PixelWriter{
  public:
    PixelWriter(const FrameBufferConfig &config): config_{config}{
    }
    virtual ~PixelWriter() = default;
    virtual void Write(int x, int y, const PixelColor &c) = 0;

  protected:
    uint8_t* PixelAt(int x, int y){
      return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
    }
  
  private:
    const FrameBufferConfig &config_;
};

class RGBResv8BitPerColorPixelWriter: public PixelWriter{
  public:
    using PixelWriter::PixelWriter;

    virtual void Write(int x, int y, const PixelColor &c) override {
      auto p = PixelAt(x,y);
      p[0] = c.r;
      p[1] = c.g;
      p[2] = c.b;
    }
};
class BGRResv8BitPerColorPixelWriter: public PixelWriter{
  public:
    using PixelWriter::PixelWriter;

    virtual void Write(int x, int y, const PixelColor &c) override {
      auto p = PixelAt(x,y);
      p[0] = c.b;
      p[1] = c.g;
      p[2] = c.r;
    }
};

void* operator new(size_t size, void *buf){
  return buf;
}
void operator delete(void *obj) noexcept{
}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer = nullptr;

extern "C" void KernelMain(const FrameBufferConfig &frame_buffer_config)
{
  switch(frame_buffer_config.pixel_format){
    case kPixelRGBResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf) RGBResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
    case kPixelBGRResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf) BGRResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
  }

  if(pixel_writer != nullptr){
    for(int x=0; x!=frame_buffer_config.horizontal_resolution; ++x){
      for(int y=0; y!=frame_buffer_config.vertical_resolution; ++y){
        pixel_writer->Write(x, y, {0xff, 0xff, 0xff});
      }
    }
    for(int x=frame_buffer_config.horizontal_resolution/3; x!=frame_buffer_config.horizontal_resolution/3 * 2; ++x){
      for(int y=frame_buffer_config.vertical_resolution/4; y!=frame_buffer_config.vertical_resolution/4*3; ++y){
          pixel_writer->Write(x, y, {0xff, 0x00, 0x00});
      }
    }
  }

  hlt();
}