#pragma once

#include<vector>
#include<optional>
#include"graphics.hpp"

// Window class has its pixel data in @data_.
// Data in @data_ member is not drawn onto graphic memory until explicitly @DrawTo().
// Window itself doesn't know its position.
class Window{
  public:
    class WindowWriter: public PixelWriter{
      public:
        WindowWriter(Window &window): window_{window} {}
        // 
        virtual void Write(int x, int y, const PixelColor &c) override{
          window_.At(x,y) = c;
        }
        virtual int Width() const override {return window_.Width();}
        virtual int Height() const override {return window_.Height();}

      private:
        Window &window_;
    };

    Window(int width, int height);
    ~Window() = default;
    Window(const Window &rhs) = delete;               // disallow construct by copying
    Window& operator=(const Window &rhs) = delete;    // disallow copying

    // draw this window onto the given @writer
    void DrawTo(PixelWriter &writer, Vector2D<int> position);
    // set transparent color
    void SetTransparentColor(std::optional<PixelColor> c);
    WindowWriter *Writer();

    PixelColor& At(int x, int y);
    const PixelColor& At(int x, int y) const;

    int Width() const;
    int Height() const;
  
  private:
    int width_, height_;
    std::vector<std::vector<PixelColor>> data_{};
    WindowWriter writer_{*this};
    std::optional<PixelColor> transparent_color_{std::nullopt};
};