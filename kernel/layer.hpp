#pragma once

#include<memory>
#include<map>
#include<vector>

#include"graphics.hpp"
#include"window.hpp"

// for now, Layer class can have only one window.
class Layer{
  public:
    Layer(unsigned id = 0);
    unsigned int ID() const;

    // set window. previously set window is unset from this layer.
    Layer& SetWindow(const std::shared_ptr<Window> &window);
    std::shared_ptr<Window> GetWindow() const;

    // update layer position. (doesn't redraw)
    Layer& Move(Vector2D<int> pos);
    Layer& MoveRelative(Vector2D<int> pos_diff);

    // draw window
    void DrawTo(PixelWriter &writer) const;
  
  private:
    unsigned id_;
    Vector2D<int> pos_;
    std::shared_ptr<Window> window_;
};

class LayerManager{
  public:
    void SetWriter(PixelWriter *writer);

    // generate new layer
    Layer& NewLayer();

    // draw visible layers
    void Draw() const;

    void Move(unsigned id, Vector2D<int> new_position);
    void MoveRelative(unsigned id, Vector2D<int> pos_diff);

    // set z-position.
    // negative numbers means unvisible.
    void UpDown(unsigned id, int new_height);
    void Hide(unsigned id);

    Layer* FindLayer(unsigned id);
  
  private:
    PixelWriter *writer_{nullptr};
    std::vector<std::unique_ptr<Layer>> layers_{};    // all layers
    std::vector<Layer*> layer_stack_{};               // visible layers
    unsigned latest_id_{0};
};

extern LayerManager *layer_manager;