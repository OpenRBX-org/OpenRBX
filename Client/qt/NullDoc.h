#ifndef NULLDOC_H
#define NULLDOC_H

#include "DocView.h"

class CNullDoc : public Document
{
public:
	CNullDoc();

	bool isModified() const override { return false; }
	bool saveModified() override { return true; }
};

#endif // NULLDOC_H
