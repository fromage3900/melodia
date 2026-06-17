# Melusina rig + mocap import helper (manual import in editor — avoid UnrealMCP bulk skeletal import)
# Rig (local): G:\Melodia\Content\Import\Melusina\WorkingMelusinaScene.fbx
# Placeholder: CC0 VRM via VRM4U -> /Game/Melodia/Characters/Placeholder/ (see Docs/CHARACTER_SETUP.md)
# Mocap: G:\Mocap\*.fbx (Auto-Rig Pro / UE5 naming — may bind direct to skeleton)

$RigFbx = "G:\Melodia\Content\Import\Melusina\WorkingMelusinaScene.fbx"
$MocapRoot = "G:\Mocap"
$ContentRoot = "/Game/Melodia/Characters/Melusina"
$SkeletonPath = "$ContentRoot/SK_Melusina_Prototype_Skeleton"  # adjust after mesh import

Write-Host "=== Mocap inventory (G:\Mocap) ==="
Get-ChildItem $MocapRoot -Filter *.fbx | Sort-Object Name | ForEach-Object {
    Write-Host "  $($_.Name)  ($([math]::Round($_.Length/1MB,2)) MB)"
}

Write-Host @"

=== Prefer manual import (see Docs/CHARACTER_SETUP.md) ===
UnrealMCP import_skeletal_mesh on the full Melusina FBX has frozen the editor.
Use VRM4U drag-drop for a placeholder, then import custom rig one file at a time.

=== Unreal MCP sequence (optional, small files only) ===
1. import_skeletal_mesh
   source_path: $RigFbx
   asset_name: SK_Melusina_Prototype
   destination_path: $ContentRoot/
   import_animations: false

2. create_anim_blueprint
   blueprint_name: ABP_Melusina
   skeleton_path: <skeleton from step 1>
   blueprint_path: $ContentRoot/

3. create_blueprint
   name: BP_Melusina_Hero
   parent_class: /Script/MelodiaMelusina_PROD.MelodiaCharacterBase

4. set_character_properties
   blueprint_path: /Game/Melodia/Characters/Melusina/BP_Melusina_Hero
   skeletal_mesh_path: $ContentRoot/SK_Melusina_Prototype
   anim_blueprint_path: $ContentRoot/ABP_Melusina
   auto_fit_capsule: true

5. import_animation (per file, 3s pause between calls)
   skeleton_path: <skeleton from step 1>
   destination_path: $ContentRoot/Animations/
   Priority: RunCycle, RunCycle_Sprint, Jump, Dodge, GracefulLanding, Twirl

6. set_game_mode_default_pawn -> BP_Melusina_Hero (optional)

=== Retargeting ===
If ARP UE5 export matches Melusina skeleton bone names: NO IK Retargeter needed.
If import warns skeleton mismatch: create IK_Melusina + RTG_MocapToMelusina and batch retarget.

=== Chaos Cloth (next) ===
- Duplicate outfit mesh to SK_Melusina_Outfit_Cloth
- Create Chaos Cloth Asset on skirt/cape sections
- Paint Max Distance at waist/seams = 0, free hem higher
- Assign on outfit component; keep body on main mesh with MooaToon materials
"@
