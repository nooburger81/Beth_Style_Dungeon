using UnrealBuildTool;
using System.Collections.Generic;

public class Beth_Style_DungeonEditorTarget : TargetRules
{
	public Beth_Style_DungeonEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Beth_Style_Dungeon");
	}
}
