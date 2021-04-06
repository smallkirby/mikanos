#include"mouse.hpp"
#include"graphics.hpp"
#include"color.hpp"

namespace{
  const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ",
    "@@             ",
    "@.@            ",
    "@..@           ",
    "@...@          ",
    "@....@         ",
    "@.....@        ",
    "@......@       ",
    "@.......@      ",
    "@........@     ",
    "@.........@    ",
    "@..........@   ",
    "@...........@  ",
    "@............@ ",
    "@......@@@@@@@@",
    "@......@       ",
    "@....@@.@      ",
    "@...@ @.@      ",
    "@..@   @.@     ",
    "@.@    @.@     ",
    "@@      @.@    ",
    "@       @.@    ",
    "         @.@   ",
    "         @@@   ",
  };
}

void DrawMouseCursor(PixelWriter *writer, Vector2D<int> pos)
{
  for(int dy=0; dy!=kMouseCursorHeight; ++dy){
    for(int dx=0; dx!=kMouseCursorWidth; ++dx){
      if(mouse_cursor_shape[dy][dx] == '@'){
        writer->Write(pos.x + dx, pos.y + dy, C_MOUSE_GRID);
      }else if(mouse_cursor_shape[dy][dx] == '.'){
        writer->Write(pos.x+ dx, pos.y + dy, C_MOUSE_NORMAL);
      }else{
        writer->Write(pos.x + dx, pos.y + dy, C_MOUSE_TRANSPARENT);
      }
    }
  }
}
