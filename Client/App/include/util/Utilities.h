#ifndef UTIL_UTILITIES_H
#define UTIL_UTILITIES_H

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace RBX {

class Debugable
{
public:
	enum AssertAction
	{
		CrashOnAssert = 0,
		IgnoreAssert = 1
	};

	static AssertAction assertAction;
	static bool validatingDebug;

	static void forceBadTypeId();
	static void doCrash();

	static void dump(void* object, std::ostream& stream);
	virtual void dump(std::ostream& stream);
};

template <class T>
class CopyOnWrite
{
public:
	CopyOnWrite(const CopyOnWrite&) = delete;
	CopyOnWrite& operator=(const CopyOnWrite&) = delete;

	CopyOnWrite() {}

	CopyOnWrite(const T& value) : object(new T(value)) {}

	explicit operator bool() const { return object != nullptr; }

	const T& operator*() const { return *object; }

	const T* operator->() const { return object.get(); }

	std::shared_ptr<const T> read() const { return object; }

	std::shared_ptr<T>& write()
	{
		if (!object) {
			object = std::shared_ptr<T>(new T());
		}
		else if (object.use_count() > 1) {
			object = std::shared_ptr<T>(new T(*object));
		}

		return object;
	}

	void reset() { object.reset(); }

private:
	std::shared_ptr<T> object;
};

template <class T>
class StringConverter
{
public:
	static std::string convertToString(const T& value);
	static bool convertToValue(const std::string& text, T& value);
};

template <class T>
T lexicalCast(const std::string& text);

std::string readStream(std::istream& stream);

void toLower(std::string& text);
void trim(std::string& text);
std::vector<std::string> split(const std::string& text, const char* delimiters);
std::vector<std::string> tokenize(const std::string& text, const char* delimiters);

} // namespace RBX

#endif // UTIL_UTILITIES_H
