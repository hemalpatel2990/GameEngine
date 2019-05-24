#ifndef PCSTREE_SET_ITERATOR_H
#define PCSTREE_SET_ITERATOR_H

#include "PCSTreeIterator.h"

class PCSTreeSetIterator : public PCSTreeIterator
{
public:

	PCSTreeSetIterator(PCSNode *rootNode);

	virtual PCSNode *First() override;
	virtual PCSNode *Next() override;
	virtual bool IsDone() override;
	virtual PCSNode *CurrentItem() override;

private:
	PCSNode *privGetNext(PCSNode *node, bool UseChild = true);

	PCSNode *root;
	PCSNode *current;
};


#endif