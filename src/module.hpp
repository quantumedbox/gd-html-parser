#pragma once

#include <optional>

#include <Godot.hpp>
#include <Variant.hpp>
#include <Node.hpp>
#include <gumbo.h>

class HTMLParser : public godot::Node {
  GODOT_CLASS(HTMLParser, godot::Node);

public:
  static void _register_methods();

  void _init();

  godot::Dictionary parse(const godot::String text) const;

private:
  // gumbo options
  int tab_stop;
  bool use_xhtml_rules;
  bool stop_on_first_error;
  unsigned int max_tree_depth;
  int max_errors;

  bool collect_whitespace;

  GumboOptions collect_options() const;
  std::optional<godot::Dictionary> parse_node(const GumboNode *const node) const;
  godot::Array parse_contents(GumboElement elem) const;
  godot::Dictionary parse_attributes(GumboElement elem) const;
};
