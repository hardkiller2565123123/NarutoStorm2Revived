NS2 Revived Asset Menu Big Update

Drop these files into steam_api64 and replace existing files. Add new .cpp files to the Visual Studio project:
- AssetModelViewer.cpp
- CpkArchive.cpp

This update keeps existing systems and adds/refines:
- Asset cache loaded on startup
- Background async asset scan on launch
- Top-right loading toast with percent/warning
- Sidebar category filters working again
- Auto-preview on selection; no Preview button required
- 3D object viewer shell with spinning preview, grid, zoom, wireframe
- Hex view restored
- Safe edit panel placeholder that writes only to Dump/Edited later
- CPK heuristic indexer included

Note: real NUD mesh rendering still requires a full NUD/XFBIN mesh decoder. The viewer is prepared and shows a spinning object shell until decoded geometry is wired.
