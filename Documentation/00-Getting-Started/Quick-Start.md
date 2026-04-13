# Quick Start Tutorial

## Your First Entity

### 1. Create a Blueprint

1. Open the **Blueprint Editor** (main window)
2. Go to **File → New** to create a new blueprint graph
3. Right-click on the canvas → **Add Node → Action → MoveToTarget**

### 2. Create an Entity Prefab

1. Go to **File → New Entity Prefab** (Ctrl+Alt+N)
2. The **Entity Prefab Editor** opens
3. From the **Component Palette**, drag a **Transform** node onto the canvas
4. Drag a **Sprite** node and connect it to Transform
5. Press **Ctrl+S** to save as `Gamedata/PrefabEntities/my_entity.json`

### 3. Create a Behavior Tree

1. Open the **BT Editor** from the editors list
2. Add a **Sequence** node as root
3. Add child nodes: **MoveToTarget** → **Attack**
4. Save as `Gamedata/BehaviorTrees/my_bt.json`

### 4. Assign BT to Prefab

1. Back in the Entity Prefab Editor
2. Add a **BehaviorTree** component node
3. Set the `graphFile` parameter to `BehaviorTrees/my_bt.json`

## Key Shortcuts

| Action | Shortcut |
|--------|----------|
| New Blueprint | Ctrl+N |
| New Entity Prefab | Ctrl+Alt+N |
| Save | Ctrl+S |
| Undo | Ctrl+Z |
| Redo | Ctrl+Y |
