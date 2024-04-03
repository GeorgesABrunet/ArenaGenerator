# ArenaGenerator
Repository for an Unreal Engine plugin, Arena Generator.

## Overview

This is an Unreal Engine plugin to create Arenas in regular polygonal shapes in a modular fashion. 
Originally made for the title [Shifting Mausoleum](https://pootpootpoot.itch.io/shifting-mausoleum), the functionality was aimed to support semi-procedural arena generation for worlds in three sections, with unique behaviors for each section.
A little more thought showed this could be built upon to support a fully modular approach to arena building. This plugin can now help you build geometrically complex structures by providing the placement logic. All you need is to bring the meshes. 

## Features

- Building grids and regular polygons vertically as sections
- Built-in generation rules to modulate building behavior for each section
- Mesh group logic to optimize instantiation
- Support for defining mesh origins in relation to generation (accounting for the origins that are not in a specific corner or center)
- Data table support for generation parametrization
- Ability to index sections to reuse in generation

## How to use it

Go to the [Getting Started](https://github.com/GeorgesABrunet/ArenaGenerator/wiki/Getting-Started) page for instructions on how to begin work with the plugin.

Details on how it works internally can be found in the Wiki, [Under The Hood](https://github.com/GeorgesABrunet/ArenaGenerator/wiki/Under-The-Hood). 

There is currently no Discord server dedicated to this plugin. Plan on changing that at some point.

## Installation

This is installed like any other Unreal Engine c++ plugin, more information can be found on the [Installation](https://github.com/GeorgesABrunet/ArenaGenerator/wiki/Installation) page of the wiki.

## License

This Plugin is under an MIT License. You are free to use this plugin for personal/free/commercial projects, you are also allowed to modify the source code and/or redistribute it.
The only condition is to add the copyright notice and a copy of the license with your project and/or any redistribution of the source code, modified or not.

## To Do List (Not in order)

- Example project
- Tutorial videos
- Ability to bake out arenas as static meshes in world
- Editor tools to ease building arenas
- Mesh patterns and custom pattern support
- Rotational offsets for RotationYP rule for polygonal structures
- Arena placement in relation to actor
- Asynchronous loading support
- Hierarchical instancing
