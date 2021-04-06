#include"graphics.hpp"
#include"color.hpp"

void RGBResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor &c)
{
  auto p = PixelAt(x,y);
  p[0] = c.r;
  p[1] = c.g;
  p[2] = c.b;
}

void BGRResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor &c)
{
  auto p = PixelAt(x,y);
  p[0] = c.b;
  p[1] = c.g;
  p[2] = c.r;
}

void FillRectangle(PixelWriter &writer, const Vector2D<int> &pos, const Vector2D<int> &size, const PixelColor &c)
{
  for(int dy=0; dy!=size.y; ++dy){
    for(int dx=0; dx!=size.x; ++dx){
      writer.Write(pos.x + dx, pos.y + dy, c);
    }
  }
}

void DrawRectangle(PixelWriter &writer, const Vector2D<int> &pos, const Vector2D<int> &size, const PixelColor &c)
{
  for(int dy=0; dy!=size.y; ++dy){
    writer.Write(pos.x + 0, pos.y + dy, c);
    writer.Write(pos.x + size.x, pos.y + dy, c);
  }
  for(int dx=0; dx!=size.x; ++dx){
    writer.Write(pos.x + dx, pos.y + 0, c);
    writer.Write(pos.x + dx, pos.y + size.y, c);
  }
}

void DrawDesktop(PixelWriter &writer)
{
  const auto width = writer.Width();
  const auto height = writer.Height();
  FillRectangle(writer, {0,0}, {width, height}, C_DESKTOP_BG);
  FillRectangle(writer, {0, height-50}, {width, 50}, C_DESKTOP_FOOTER);
  FillRectangle(writer, {0, height-50 + 3}, {height/5, 50-3-3}, C_DESKTOP_ACCENT);
  DrawRectangle(writer, {0, height-50 + 3}, {height/5, 50-3-3}, C_DESKTOP_EDGE);
}
