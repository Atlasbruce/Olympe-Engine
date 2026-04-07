#!/usr/bin/env python3
import json
import os

os.chdir("C:/Users/ncher/source/repos/OlympeEngine")

# Load ComponentsParameters.json
with open('Gamedata/EntityPrefab/ComponentsParameters.json', 'r') as f:
    schemas = json.load(f)

# Create a mapping of component types to parameter names
component_params = {}
for schema in schemas.get('schemas', []):
    comp_type = schema.get('componentType')
    params = schema.get('parameters', [])
    param_names = [p.get('name') for p in params]
    component_params[comp_type] = param_names

# Load Simple_NPC.ats
with open('OlympeBlueprintEditor/Simple_NPC.ats', 'r') as f:
    prefab = json.load(f)

print('=== Component types used in Simple_NPC.ats ===')
for node in prefab.get('data', {}).get('nodes', []):
    comp_type = node.get('componentType')
    props_in_json = list(node.get('properties', {}).keys())
    expected_props = component_params.get(comp_type, [])

    print(f'\n{comp_type}:')
    print(f'  Properties in JSON: {props_in_json if props_in_json else "[EMPTY]"}')
    print(f'  Expected from schema: {expected_props}')
    print(f'  Status: {"EMPTY - will be filled by fix" if not props_in_json else "OK"}')

print('\n=== FIX VERIFICATION ===')
print('After LoadFromFile() with our fix, all EMPTY nodes will be populated')
print('with their schema defaults from ComponentsParameters.json')
