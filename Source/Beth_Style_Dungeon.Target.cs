using UnrealBuildTool;
using System.Collections.Generic;

public class Beth_Style_DungeonTarget : TargetRules
{
	public Beth_Style_DungeonTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Beth_Style_Dungeon");
	}
}
