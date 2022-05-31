#include "module.hpp"

#include <cassert>
#include <cstddef>
#include <optional>

#include <Godot.hpp>
#include <Variant.hpp>
#include "Dictionary.hpp"
#include "GodotGlobal.hpp"
#include "String.hpp"
#include <gumbo.h>
#include <string_buffer.h>
#include <error.h>

extern "C" void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options *o) {
  godot::Godot::gdnative_init(o);
}

extern "C" void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options *o) {
  godot::Godot::gdnative_terminate(o);
}

extern "C" void GDN_EXPORT godot_nativescript_init(void *handle) {
  godot::Godot::nativescript_init(handle);
  godot::register_class<HTMLParser>();
}

void HTMLParser::_register_methods() {
  godot::register_method("parse", &HTMLParser::parse);

  godot::register_property<HTMLParser, int>("tab_stop", &HTMLParser::tab_stop, kGumboDefaultOptions.tab_stop);
  godot::register_property<HTMLParser, bool>("use_xhtml_rules", &HTMLParser::use_xhtml_rules, kGumboDefaultOptions.use_xhtml_rules);
  godot::register_property<HTMLParser, bool>("stop_on_first_error", &HTMLParser::stop_on_first_error, kGumboDefaultOptions.stop_on_first_error);
  godot::register_property<HTMLParser, unsigned int>("max_tree_depth", &HTMLParser::max_tree_depth, kGumboDefaultOptions.max_tree_depth);
  godot::register_property<HTMLParser, int>("max_errors", &HTMLParser::max_errors, kGumboDefaultOptions.max_errors);
  godot::register_property<HTMLParser, bool>("collect_whitespace", &HTMLParser::collect_whitespace, false);
}

void HTMLParser::_init() {
  tab_stop = kGumboDefaultOptions.tab_stop;
  use_xhtml_rules = kGumboDefaultOptions.use_xhtml_rules;
  stop_on_first_error = kGumboDefaultOptions.stop_on_first_error;
  max_tree_depth = kGumboDefaultOptions.max_tree_depth;
  max_errors = kGumboDefaultOptions.max_errors;
}

godot::Array HTMLParser::parse_contents(GumboElement elem) const {
  godot::Array result;

  GumboVector children = elem.children;
  for (unsigned int i = 0; i < children.length; ++i) {
    GumboNode *child = static_cast<GumboNode*>(children.data[i]);
    if (auto parse_result = this->parse_node(child))
      result.append(parse_result.value());
  }

  return result;
}

godot::Dictionary HTMLParser::parse_attributes(GumboElement elem) const {
  godot::Dictionary result;

  GumboVector attribs = elem.attributes;
  for (unsigned int i = 0; i < attribs.length; ++i) {
    GumboAttribute *attrib = static_cast<GumboAttribute*>(attribs.data[i]);
    result[attrib->name] = attrib->value;
  }

  return result;
}

std::optional<godot::Dictionary> HTMLParser::parse_node(const GumboNode *const node) const {
  godot::Dictionary result;

  bool could_have_children = false;
  bool could_have_attributes = false;

  switch (node->type) {
    case GUMBO_NODE_DOCUMENT: {
      result["type"] = "document";
      // todo: Get its name?
      if (node->v.document.has_doctype) {
        godot::String public_identifier{node->v.document.public_identifier};
        if (!public_identifier.empty())
          result["public_identifier"] = public_identifier;
        godot::String system_identifier{node->v.document.system_identifier};
        if (!system_identifier.empty())
          result["system_identifier"] = system_identifier;
      }
      could_have_children = true;
      break;
    }
    case GUMBO_NODE_ELEMENT: {
      result["type"] = "element";
      result["tag"] = gumbo_normalized_tagname(node->v.element.tag);
      could_have_children = true;
      could_have_attributes = true;
      break;
    }
    case GUMBO_NODE_TEMPLATE: {
      result["type"] = "template";
      result["tag"] = gumbo_normalized_tagname(node->v.element.tag);
      could_have_children = true;
      could_have_attributes = true;
      break;
    }
    case GUMBO_NODE_TEXT: {
      result["type"] = "text";
      result["text"] = node->v.text.text;
      break;
    }
    case GUMBO_NODE_COMMENT: {
      result["type"] = "comment";
      result["text"] = node->v.text.text;
      break;
    }
    case GUMBO_NODE_CDATA: {
      result["type"] = "cdata";
      result["text"] = node->v.text.text;
      break;
    }
    case GUMBO_NODE_WHITESPACE: {
      if (this->collect_whitespace) {
        result["type"] = "whitespace";
        result["text"] = node->v.text.text; // todo: Should it capture original input instead?
      }
      else return {}; // Nothing
      break;
    }
    default: {
      godot::Godot::print_warning("Unkown node type encountered", __func__, __FILE__, __LINE__);
    }
  }

  if (could_have_children) {
    auto contents = this->parse_contents(node->v.element);
    if (!contents.empty())
      result["children"] = contents;
  }

  if (could_have_attributes) {
    auto attribs = this->parse_attributes(node->v.element);
    if (!attribs.empty())
      result["attributes"] = attribs;
  }

  return {result};
}

GumboOptions HTMLParser::collect_options() const {
  return GumboOptions{
    tab_stop,
    use_xhtml_rules,
    stop_on_first_error,
    max_tree_depth,
    max_errors
  };
}

godot::Dictionary HTMLParser::parse(const godot::String text) const {
  godot::CharString ch_str = text.ascii();

  GumboOptions opts = collect_options();
  GumboOutput* output = gumbo_parse_with_options(&opts, ch_str.get_data(), ch_str.length());

  // todo: Should return HTMLParserError object instead, containing error info
  if (output->status != GUMBO_STATUS_OK) {
    GumboVector errors = output->errors;
    for (unsigned int i = 0; i < errors.length; ++i) {
      GumboError *err = static_cast<GumboError*>(errors.data[i]);
      GumboStringBuffer err_msg;
      gumbo_string_buffer_init(&err_msg);
      gumbo_error_to_string(err, &err_msg);
      godot::Godot::print_error(err_msg.data, __func__, __FILE__, __LINE__);
      gumbo_string_buffer_destroy(&err_msg);
    }
    return godot::Dictionary{};
  }

  godot::Dictionary result;

  if (auto res = this->parse_node(output->document))
    result = {res.value()};

  gumbo_destroy_output(output);
  return result;
}
