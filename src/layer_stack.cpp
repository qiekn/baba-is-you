#include "layer_stack.h"

#include <algorithm>

LayerStack::~LayerStack() { clear(); }

void LayerStack::push_layer(std::unique_ptr<Layer> layer) {
  layer->OnAttach();
  layers_.insert(layers_.begin() + overlay_start_, std::move(layer));
  ++overlay_start_;
}

void LayerStack::push_overlay(std::unique_ptr<Layer> overlay) {
  overlay->OnAttach();
  layers_.push_back(std::move(overlay));
}

void LayerStack::pop_layer(Layer* layer) {
  auto last = layers_.begin() + overlay_start_;
  auto it = std::find_if(layers_.begin(), last,
                         [layer](const auto& p) { return p.get() == layer; });
  if (it == last) return;
  (*it)->OnDetach();
  layers_.erase(it);
  --overlay_start_;
}

void LayerStack::pop_overlay(Layer* overlay) {
  auto first = layers_.begin() + overlay_start_;
  auto it = std::find_if(first, layers_.end(),
                         [overlay](const auto& p) { return p.get() == overlay; });
  if (it == layers_.end()) return;
  (*it)->OnDetach();
  layers_.erase(it);
}

void LayerStack::clear() {
  for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
    (*it)->OnDetach();
  }
  layers_.clear();
  overlay_start_ = 0;
}
