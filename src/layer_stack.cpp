#include "layer_stack.h"

LayerStack::~LayerStack() { Clear(); }

void LayerStack::PushLayer(std::unique_ptr<Layer> layer) {
  layer->OnAttach();
  layers_.insert(layers_.begin() + overlay_start_, std::move(layer));
  ++overlay_start_;
}

void LayerStack::PushOverlay(std::unique_ptr<Layer> overlay) {
  overlay->OnAttach();
  layers_.push_back(std::move(overlay));
}

void LayerStack::Clear() {
  for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
    (*it)->OnDetach();
  }
  layers_.clear();
  overlay_start_ = 0;
}
