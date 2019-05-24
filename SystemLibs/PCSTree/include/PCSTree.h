#ifndef PCSTREE_H
#define PCSTREE_H

// Simple data structure
struct PCSTreeInfo
{
	int currNumNodes;
	int maxNumNodes;
	int currNumLevels;
	int maxNumLevels;
};

// forward declare
class PCSNode;

// PCSTree class 
class PCSTree
{
public:
	// constructor
	PCSTree();

	// destructor
	~PCSTree();

	// get Root
	PCSNode *getRoot(void) const;

	// insert
	void insert(PCSNode * const inNode, PCSNode * const parent);

	// remove
	void remove(PCSNode * const inNode);

	// get info
	void getInfo(PCSTreeInfo &info);
	void printTree() const;

private:
	// update info
	void privInfoAddNode();
	void privInfoRemoveNode();
	void privInfoAddNewLevel(PCSNode * const inNode);
	void privInfoRemoveNewLevel(PCSNode * const inNode);

	// copy constructor 
	PCSTree(const PCSTree &in);

	// assignment operator
	PCSTree & operator = (const PCSTree &in);

	// Data
	PCSTreeInfo mInfo;
	PCSNode     *root;

	// safety
	void privDepthFirst(PCSNode *tmp);
	void privDumpTreeDepthFirst(PCSNode *node) const;
	void privRecalculateLevel(PCSNode *node);
};



#endif