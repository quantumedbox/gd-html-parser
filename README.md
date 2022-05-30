# gd-gumbo HTMLParser
Simple implementation of html/xhtml document walker that outputs node tree representation in form of Godot Dictionary object.

## Requirements for building
- SCons
- CMake (for sigil-gumbo)
- C/C++ Compiler toolchain (for now only GCC/MinGW and Clang/Clang++ are supported out of the box)

## Building steps
### Windows
Currently readymade solution is only available under MinGW package be it MSYS2 or Winlibs or else.

Example of command:
`scons platform=windows bits=64 use_mingw=on target=release`
This will automatically build sigil-gumbo, godot-cpp and gd-gumbo sources at the same time. You're required to pass either `use_mingw=on` or `use_llvm=on` for build system to know compilers you're using. Look into `SConstruct.py` to get all the options available.

## Using in your Godot project
After building you should have `bin` folder that has dynamically loadable objects for platforms you compiled. You can copy it straight in the root of your project and use `res://bin/gd-gumbo.gdns` script for instancing parser class.

Example:
```gdscript
extends Node

const example := """
<!DOCTYPE html>
<html>
<head>
   <meta charset="UTF-8">
</head>
<body>
  <h1 class="my-style">My First Heading</h1>
  <p attrib="something" another-one="hello world!">My first paragraph.</p>
</body>
</html>
"""

func _ready() -> void:
  var parser_instance := preload("res://bin/gd-gumbo.gdns").new()
  parser_instance.stop_on_first_error = true # Currently empty dict will be returned on error, this behavior could be changed in the future
  pretty_print(parser_instance.parse(example))
  parser_instance.queue_free()


static func pretty_print(obj, indent: int = 0):
  if obj is Dictionary:
    for item in obj:
      if obj[item] is Dictionary or obj[item] is Array:
        print(" ".repeat(indent) + item + ": ")
        pretty_print(obj[item], indent + 1)
      else:
        print(" ".repeat(indent) + item + ": " + obj[item])
  elif obj is Array:
    for item in obj:
      if item is Dictionary or item is Array:
        pretty_print(item, indent + 1)
      else:
        print(" ".repeat(indent) + item)
  else: assert(false)
```

Output:
```
type: document
children:
  type: element
  tag: html
  children:
    type: element
    tag: head
    children:
      type: element
      tag: meta
      attributes:
       charset: UTF-8
    type: element
    tag: body
    children:
      type: element
      tag: h1
      children:
        type: text
        text: My First Heading
      attributes:
       class: my-style
      type: element
      tag: p
      children:
        type: text
        text: My first paragraph.
      attributes:
       attrib: something
       another-one: hello world!
```
