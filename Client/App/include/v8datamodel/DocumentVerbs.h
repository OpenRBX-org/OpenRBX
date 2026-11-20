#ifndef V8DATAMODEL_DOCUMENTVERBS_H
#define V8DATAMODEL_DOCUMENTVERBS_H

#include "tool/GrabTool.h"
#include "tool/NullTool.h"
#include "tool/ToolsArrow.h"
#include "tool/ToolsSurface.h"
#include "v8datamodel/Commands.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/ToolsPart.h"

namespace RBX {

class DocumentVerbs
{
public:
	DocumentVerbs(DataModel* dataModel);

	PlayDeleteSelectionVerb m_playDeleteSelection;
	DeleteSelectionVerb m_deleteSelection;
	SelectAllCommand m_selectAll;
	GroupSelectionVerb m_groupSelection;
	UngroupSelectionVerb m_ungroupSelection;
	WeldSelectionVerb m_weldSelection;
	SnapSelectionVerb m_snapSelection;
	DuplicateSelectionVerb m_duplicateSelection;
	UndoVerb m_undo;
	RedoVerb m_redo;
	AttachCameraCommand m_attachCamera;
	TrackCameraCommand m_trackCamera;
	FollowCameraCommand m_followCamera;
	WatchCameraCommand m_watchCamera;
	AnchorVerb m_anchor;
	TranslucentVerb m_translucent;
	CanCollideVerb m_canCollide;
	CanNotSelectCommand m_canNotSelect;
	AllCanSelectCommand m_allCanSelect;
	MoveUpPlateVerb m_moveUpPlate;
	MoveUpBrickVerb m_moveUpBrick;
	MoveDownSelectionVerb m_moveDownSelection;
	RotateSelectionVerb m_rotateSelection;
	TiltSelectionVerb m_tiltSelection;
	PrimaryControllerCommand m_primaryController;
	SecondaryControllerCommand m_secondaryController;
	ClearStarterpack m_clearStarterpack;
	ClearBackpack m_clearBackpack;
	RunCommand m_run;
	StopCommand m_stop;
	ResetCommand m_reset;
	FirstPersonCommand m_firstPerson;
	StatsCommand m_stats;
	ForceAssertionCommand m_forceAssertion;
	EngineStatsCommand m_engineStats;
	JoinCommand m_join;
	TToolVerb<ArrowTool, RunStateVerb> m_arrowTool;
	TToolVerb<ResizeTool, RunStateVerb> m_resizeTool;
	TToolVerb<AxisMoveTool, RunStateVerb> m_axisMoveTool;
	TToolVerb<FlatTool, RunStateVerb> m_flatTool;
	TToolVerb<GlueTool, RunStateVerb> m_glueTool;
	TToolVerb<WeldTool, RunStateVerb> m_weldTool;
	TToolVerb<StudsTool, RunStateVerb> m_studsTool;
	TToolVerb<InletTool, RunStateVerb> m_inletTool;
	TToolVerb<HingeTool, RunStateVerb> m_hingeTool;
	TToolVerb<RightMotorTool, RunStateVerb> m_rightMotorTool;
	TToolVerb<LeftMotorTool, RunStateVerb> m_leftMotorTool;
	TToolVerb<OscillateMotorTool, RunStateVerb> m_oscillateMotorTool;
	TToolVerb<ModelSetFrontTool, RunStateVerb> m_modelSetFrontTool;
	TToolVerb<ModelSetPrimaryPartTool, RunStateVerb> m_modelSetPrimaryPartTool;
	TToolVerb<AnchorTool, RunStateVerb> m_anchorTool;
	TToolVerb<LockTool, RunStateVerb> m_lockTool;
	TToolVerb<FillTool, RunStateVerb> m_fillTool;
	TToolVerb<DropperTool, RunStateVerb> m_dropperTool;
	TToolVerb<NullTool, RunStateVerb> m_nullTool;
	TToolVerb<GameTool, RunStateVerb> m_gameTool;
	TToolVerb<GrabTool, RunStateVerb> m_grabTool;
	TToolVerb<CloneTool, RunStateVerb> m_cloneTool;
	TToolVerb<HammerTool, RunStateVerb> m_hammerTool;
	TToolVerb<SlingshotTool, RunStateVerb> m_slingshotTool;
	TToolVerb<RocketTool, RunStateVerb> m_rocketTool;
	TToolVerb<LaserTool, RunStateVerb> m_laserTool;
};

} // namespace RBX

#endif // V8DATAMODEL_DOCUMENTVERBS_H
