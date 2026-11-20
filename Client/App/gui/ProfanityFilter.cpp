#include "gui/ProfanityFilter.h"

#include "util/ContentProvider.h"
#include "util/Utilities.h"

#include <fstream>
#include <vector>

namespace RBX {

bool WordList::ContainsProfanity(std::string text)
{
	toLower(text);
	return blacklist.contains(text);
}

WordList::~WordList() = default;

WordList::WordList()
{
	std::string fileName = ContentProvider::singleton().getAssetFile("Fonts\\diogenes.fnt");
	std::ifstream file(fileName.c_str());
	std::string line;

	while (std::getline(file, line, '\n')) {
		toLower(line);
		trim(line);
		blacklist.insert(line);
	}

	file.close();
}

ProfanityFilter::ProfanityFilter() = default;

bool ProfanityFilter::ContainsProfanity(std::string text)
{
	toLower(text);

	std::vector<std::string> words = split(text, " !.?,:;><[]{}|\\/@#$%^&*()_-+=\"");

	for (unsigned int i = 0; i < words.size(); i++) {
		if (wordlist.ContainsProfanity(words[i])) {
			return true;
		}
	}

	return false;
}

ProfanityFilter::~ProfanityFilter() = default;

} // namespace RBX
