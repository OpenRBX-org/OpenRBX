#ifndef IMAGEEDITORDIALOG_H
#define IMAGEEDITORDIALOG_H

#include "Dialog.h"

#include <QImage>
#include <deque>

class EditorPicker;
class EditorPicture;
class EditorPreview;
class EditorToolBar;

class ImageEditorDialog : public CDialog
{
public:
	enum Tool
	{
		ToolPencil,
		ToolFill,
		ToolGetColor,
		ToolLine,
		ToolRectangle,
		ToolCircle,
	};

	ImageEditorDialog(const QImage& image, bool alpha, QWidget* parent);

	QImage image() const { return m_image; }
	bool alpha() const { return m_alpha; }
	bool hasImage() const { return !m_image.isNull(); }

	QSize pictureSize() const { return m_size; }
	Tool tool() const { return m_tool; }
	QRgb selectedColor() const;
	QRgb pixel(QPoint point) const;
	void pickColor(QRgb color);
	void beginChange();
	void setImage(const QImage& image);
	void commitChange();

protected:
	bool OnInitDialog() override;

private:
	void selectPicker(EditorPicker* picker);
	void command(unsigned id);
	void updateCommands();
	void undo();
	void redo();
	void copy();
	void paste();
	void load();
	void move(QPoint offset);

	QSize m_size;
	QImage m_image;
	bool m_alpha;
	QImage m_before;
	std::deque<QImage> m_undo;
	std::deque<QImage> m_redo;
	Tool m_tool = ToolPencil;
	int m_previousTool = -1;
	EditorPicker* m_selected = nullptr;
	EditorPicker* m_picker = nullptr;
	EditorPicture* m_picture = nullptr;
	EditorPreview* m_preview = nullptr;
	EditorToolBar* m_tools = nullptr;
	EditorToolBar* m_move = nullptr;
};

#endif // IMAGEEDITORDIALOG_H
