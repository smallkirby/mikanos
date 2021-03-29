#include"mouse.hpp"
#include"graphics.hpp"
#include"color.hpp"

namespace{
  const int kMouseCursorWidth = 15;
  const int kMouseCursorHeight = 24;
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

  void DrawMouseCursor(PixelWriter *writer, Vector2D<int> pos)
  {
    for(int dy=0; dy!=kMouseCursorHeight; ++dy){
      for(int dx=0; dx!=kMouseCursorWidth; ++dx){
        if(mouse_cursor_shape[dy][dx] == '@'){
          writer->Write(pos.x + dx, pos.y + dy, C_MOUSE_GRID);
        }else if(mouse_cursor_shape[dy][dx] == '.'){
          writer->Write(pos.x+ dx, pos.y + dy, C_MOUSE_NORMAL);
        }
      }
    }
  }

  void EraseMouseCursor(PixelWriter *writer, Vector2D<int> pos, PixelColor erase_color)
  {
    for(int dy=0; dy!=kMouseCursorHeight; ++dy){
      for(int dx=0; dx!=kMouseCursorWidth; ++dx){
        if(mouse_cursor_shape[dy][dx] != ' '){
          writer->Write(pos.x + dx, pos.y + dy, erase_color);
        }
      }
    }
  }
}

MouseCursor::MouseCursor(PixelWriter *writer, PixelColor erase_color, Vector2D<int> initial_position)
  : pixel_writer_{writer}, erase_color_{erase_color}, pos_{initial_position}
{
  DrawMouseCursor(pixel_writer_, pos_);
}

void MouseCursor::MoveRelative(Vector2D<int> displacement)
{
  EraseMouseCursor(pixel_writer_, pos_, erase_color_);
  pos_ += displacement;
  DrawMouseCursor(pixel_writer_, pos_);
}