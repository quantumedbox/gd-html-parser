#include "module.hpp"
#include "Dictionary.hpp"
#include "GodotGlobal.hpp"
#include "String.hpp"

#include <cassert>

#include <Godot.hpp>
#include <Variant.hpp>
#include <cstddef>
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
  godot::register_method("parse", &HTMLParser::parse);
}

void HTMLParser::_register_methods() {
  register_method("parse", &HTMLParser::parse);
}

void HTMLParser::_init() {
  //
}

static godot::Dictionary parse_node(GumboNode *node);

static godot::Array parse_contents(GumboElement elem) {
  godot::Array result;

  GumboVector children = elem.children;
  for (unsigned int i = 0; i < children.length; ++i) {
    GumboNode *child = static_cast<GumboNode*>(children.data[i]);
    result.append(parse_node(child));
  }

  return result;
}

static godot::Dictionary parse_attributes(GumboElement elem) {
  godot::Dictionary result;

  GumboVector attribs = elem.attributes;
  for (unsigned int i = 0; i < attribs.length; ++i) {
    GumboAttribute *attrib = static_cast<GumboAttribute*>(attribs.data[i]);
    result[attrib->name] = attrib->value;
  }

  return result;
}

static godot::Dictionary parse_node(GumboNode *node) {
  godot::Dictionary result;

  bool could_have_children = false;
  bool could_have_attributes = false;

  switch (node->type) {
    case GUMBO_NODE_DOCUMENT: {
      result["type"] = "document"; // todo: Technically document is `element` by HTML spec
      if (node->v.document.has_doctype) {
        godot::String public_identifier{node->v.document.public_identifier};
        if (!public_identifier.empty())
          result["public_identifier"] = public_identifier;
        godot::String system_identifier{node->v.document.system_identifier};
        if (!system_identifier.empty())
          result["system_identifier"] = system_identifier;
      }
      could_have_children = true;
      // could_have_attributes = true;
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
    default: {
      //
    }
  }

  if (could_have_children) {
    auto contents = parse_contents(node->v.element);
    if (!contents.empty())
      result["children"] = contents;
  }

  if (could_have_attributes) {
    auto attribs = parse_attributes(node->v.element);
    if (!attribs.empty())
      result["attributes"] = attribs;
  }

  return result;
}

godot::Dictionary HTMLParser::parse(const godot::String text) {
  godot::CharString ch_str = text.ascii();

  // todo: Provide options in HTMLParser object instance from Godot
  GumboOptions opts = kGumboDefaultOptions;
  opts.stop_on_first_error = false;
  GumboOutput* output = gumbo_parse_with_options(&opts, ch_str.get_data(), ch_str.length());

  if (output->status != GUMBO_STATUS_OK) {
    GumboVector errors = output->errors;
    for (unsigned int i = 0; i < errors.length; ++i) {
      GumboError *err = static_cast<GumboError*>(errors.data[i]);
      GumboStringBuffer err_msg;
      gumbo_string_buffer_init(&err_msg);
      gumbo_error_to_string(err, &err_msg);
      godot::Godot::print_error(err_msg.data, "parse", "HTMLParser", 0);
      gumbo_string_buffer_destroy(&err_msg);
    }
    return godot::Dictionary{}; // todo: Error value
  }

  godot::Dictionary result{parse_node(output->document)};

  gumbo_destroy_output(output);
  return result;
}
