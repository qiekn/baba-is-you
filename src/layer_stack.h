#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "layer.h"

// Layers update/render in insertion order; overlays always sit on top of layers
// and are iterated last. Events (if added later) should iterate in reverse so
// overlays see them first.
class LayerStack {
 public:
  LayerStack() = default;
  ~LayerStack();

  LayerStack(const LayerStack&) = delete;
  LayerStack& operator=(const LayerStack&) = delete;

  void push_layer(std::unique_ptr<Layer> layer);
  void push_overlay(std::unique_ptr<Layer> overlay);

  // Detach + destroy the matching layer. The Layer* is the handle returned
  // by push_*; passing a pointer that isn't in the stack is a no-op.
  void pop_layer(Layer* layer);
  void pop_overlay(Layer* overlay);

  // Detach and destroy every layer in reverse order. Call this before tearing
  // down resources that layers depend on (GL context, window, etc.).
  void clear();

  auto begin() { return layers_.begin(); }
  auto end() { return layers_.end(); }
  auto rbegin() { return layers_.rbegin(); }
  auto rend() { return layers_.rend(); }

  auto begin() const { return layers_.begin(); }
  auto end() const { return layers_.end(); }
  auto rbegin() const { return layers_.rbegin(); }
  auto rend() const { return layers_.rend(); }

 private:
  std::vector<std::unique_ptr<Layer>> layers_;
  std::size_t overlay_start_ = 0;
};
