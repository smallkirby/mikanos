#pragma once

#include"graphics.hpp"

// Console class holds own buffer and store console strings in it.
// It also holds PixelWriter.
class Console {
  public:
    static const int kRows = 25;    // vertical size of console
    static const int kColumns = 80; // horizontal size of console

    Console(const PixelColor &fg_color, const PixelColor &bg_color);
    void PutString(const char *s);
    void SetWriter(PixelWriter *writer);
    void Clear(void);
  
  private:
    void Newline();
    void Refresh();

    PixelWriter *writer_;
    const PixelColor fg_color_, bg_color_;
    char buffer_[kRows][kColumns + 1];
    int cursor_row_, cursor_column_;
};