#include"font.hpp"

extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

const uint8_t *GetFont(char c)
{
  auto index = 16 * static_cast<unsigned int>(c);
  if(index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)){
    return nullptr;
  }
  return &_binary_hankaku_bin_start + index;
}

// write ascii char directly onto graphic memory held by PixelWriter.
void WriteAscii(PixelWriter &writer, int x,int y, char c, const PixelColor &color)
{
  const uint8_t *font = GetFont(c);
  if(font == nullptr){
    return;
  }
  for(int dy=0; dy!=16; ++dy){
    for(int dx=0; dx!=8; ++dx){
      if((font[dy] << dx) & 0x80u){
        writer.Write(x + dx, y + dy, color);
      }
    }
  }
}

void WriteString(PixelWriter &writer, int x, int y, const char *s, const PixelColor &color)
{
  for(int ix=0; s[ix]!='\x00'; ++ix){
    WriteAscii(writer, x + 8*ix, y, s[ix], color);
  }
}
