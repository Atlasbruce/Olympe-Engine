import re

path = r'C:\Users\Nico\source\repos\Atlasbruce\Olympe-Engine\OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj'

with open(path, 'r', encoding='utf-8') as f:
    content = f.read()
    
# Add .cpp file
cpp_pattern = r'(    <ClCompile Include="\.\.\\Source\\Editor\\Modals\\SubGraphFilePickerModal\.cpp" />)'
cpp_replacement = r'\1\n    <ClCompile Include="..\\Source\\Editor\\Modals\\BehaviorTreeFilePickerModal.cpp" />'
content = re.sub(cpp_pattern, cpp_replacement, content)

# Add .h file  
h_pattern = r'(    <ClInclude Include="\.\.\\Source\\Editor\\Modals\\SubGraphFilePickerModal\.h" />)'
h_replacement = r'\1\n    <ClInclude Include="..\\Source\\Editor\\Modals\\BehaviorTreeFilePickerModal.h" />'
content = re.sub(h_pattern, h_replacement, content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
    
print('Project file updated successfully')
