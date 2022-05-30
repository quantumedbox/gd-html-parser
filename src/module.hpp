#pragma once

#include <Godot.hpp>
#include <Variant.hpp>
#include <Node.hpp>

class HTMLParser : public godot::Node {
  GODOT_CLASS(HTMLParser, godot::Node);

public:
  static void _register_methods();

  void _init();

  godot::Dictionary parse(const godot::String text);
};
