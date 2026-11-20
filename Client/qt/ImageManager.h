#ifndef IMAGEMANAGER_H
#define IMAGEMANAGER_H

#include <QIcon>
#include <map>

class ImageManager
{
public:
	static ImageManager& instance();

	bool loadToolBar(unsigned toolBarId);
	void loadBrowserImages();
	void loadCommandImages();
	QIcon icon(unsigned id) const;
	bool isAlpha(unsigned id) const;

private:
	void loadImage(unsigned id, const char* name);

	std::map<unsigned, QIcon> m_icons;
	std::map<unsigned, bool> m_alpha;
};

#endif // IMAGEMANAGER_H
