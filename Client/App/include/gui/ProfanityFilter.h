#ifndef GUI_PROFANITYFILTER_H
#define GUI_PROFANITYFILTER_H

#include <set>
#include <string>

namespace RBX {

class WordList
{
private:
	std::set<std::string> blacklist;

public:
	WordList();
	~WordList();

	bool ContainsProfanity(std::string text);
};

class ProfanityFilter
{
private:
	WordList wordlist;

public:
	ProfanityFilter();
	~ProfanityFilter();

	bool ContainsProfanity(std::string text);
};

} // namespace RBX

#endif // GUI_PROFANITYFILTER_H
