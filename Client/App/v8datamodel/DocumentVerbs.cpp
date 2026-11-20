#include "v8datamodel/DocumentVerbs.h"

namespace RBX {

DocumentVerbs::DocumentVerbs(DataModel* dataModel)
	: m_playDeleteSelection(dataModel), m_deleteSelection(dataModel), m_selectAll(dataModel),
	  m_groupSelection(dataModel), m_ungroupSelection(dataModel), m_weldSelection(dataModel),
	  m_snapSelection(dataModel), m_duplicateSelection(dataModel), m_undo(dataModel), m_redo(dataModel),
	  m_attachCamera(dataModel), m_trackCamera(dataModel), m_followCamera(dataModel), m_watchCamera(dataModel),
	  m_anchor(dataModel), m_translucent(dataModel), m_canCollide(dataModel), m_canNotSelect(dataModel),
	  m_allCanSelect(dataModel), m_moveUpPlate(dataModel), m_moveUpBrick(dataModel), m_moveDownSelection(dataModel),
	  m_rotateSelection(dataModel), m_tiltSelection(dataModel), m_primaryController(dataModel),
	  m_secondaryController(dataModel), m_clearStarterpack(dataModel), m_clearBackpack(dataModel), m_run(dataModel),
	  m_stop(dataModel), m_reset(dataModel), m_firstPerson(dataModel), m_stats(dataModel), m_forceAssertion(dataModel),
	  m_engineStats(dataModel), m_join(dataModel), m_arrowTool(dataModel), m_resizeTool(dataModel),
	  m_axisMoveTool(dataModel), m_flatTool(dataModel), m_glueTool(dataModel), m_weldTool(dataModel),
	  m_studsTool(dataModel), m_inletTool(dataModel), m_hingeTool(dataModel), m_rightMotorTool(dataModel),
	  m_leftMotorTool(dataModel), m_oscillateMotorTool(dataModel), m_modelSetFrontTool(dataModel),
	  m_modelSetPrimaryPartTool(dataModel), m_anchorTool(dataModel), m_lockTool(dataModel), m_fillTool(dataModel),
	  m_dropperTool(dataModel), m_nullTool(dataModel), m_gameTool(dataModel), m_grabTool(dataModel),
	  m_cloneTool(dataModel), m_hammerTool(dataModel), m_slingshotTool(dataModel), m_rocketTool(dataModel),
	  m_laserTool(dataModel)
{
}

} // namespace RBX
