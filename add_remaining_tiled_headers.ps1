# Script to add remaining Tiled header files to vcxproj
$vcxprojPath = "OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj"
$content = Get-Content $vcxprojPath -Raw

# Find the line with TilesetCache.h and add new headers after
$searchPattern = '(\s+<ClInclude Include="\.\.\\Source\\TiledLevelLoader\\include\\TilesetCache\.h" />)'

# Create replacement with new header files
$tiledHeaders = @'
    <ClInclude Include="..\Source\TiledLevelLoader\include\TilesetCache.h" />
    <ClInclude Include="..\Source\TiledLevelLoader\include\TilesetParser.h" />
    <ClInclude Include="..\Source\TiledLevelLoader\include\TiledDecoder.h" />
'@

# Replace
$newContent = $content -replace $searchPattern, $tiledHeaders

# Save
Set-Content $vcxprojPath -Value $newContent -NoNewline -Encoding UTF8

Write-Host "TilesetParser.h and TiledDecoder.h added successfully"
