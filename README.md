# ArenaGenerator
Repository for an Unreal Engine plugin, Arena Generator.

## Overview

This is an Unreal Engine plugin to create Arenas in regular polygonal shapes in a modular fashion. 
You can either create a conventional 3-section arena (floor, walls, roof) and define the rules for all sections,
or you can provide a set of patterns to build out sequentially.
Originally made for the title Shifting Mausoleum, the functionality was aimed to support semi-procedural arena generation for worlds in three sections, with unique behaviors for each section.
A little more thought showed this could be built upon to support a fully modular approach to arena building.

## How to use it

Details on how it works internally can be found in the Wiki. 

Go to the Getting Started page for instructions on how to begin work with the plugin.

There is currently no Discord server dedicated to this plugin. Plan on changing that at some point.

## Installation

This is installed like any other Unreal Engine c++ plugin, more information can be found on the Installation page of the wiki.

## License

This Plugin is under an MIT License. You are free to use this plugin for personal/free/commercial projects, you are also allowed to modify the source code and/or redistribute it.
The only condition is to add the copyright notice and a copy of the license with your project and/or any redistribution of the source code, modified or not.

## To Do List (Not in order)

- Base Cases Arena Generator
- Systemize overlapping behaviors in base cases for section pattern definition
- Pattern-based Arena Generator
- Support for reading patterns from data tables
- Ability to convert results to static mesh actors in level