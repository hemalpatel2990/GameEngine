#ifndef PCSNODE_H
#define PCSNODE_H

#define PCSNODE_NAME_SIZE (20)
#define PCSNODE_VERSION 2.1

// Return codes
enum PCSNodeReturnCode
{
	PCSNode_SUCCESS,
	PCSNode_FAIL_NULL_PTR,
	PCSNode_FAIL_RETURN_NOT_INITIALIZED,
	PCSNode_DWORD = 0x7fffffff
};

class PCSNode
{
public:
	// constructor
	PCSNode();

	// copy constructor
	PCSNode(const PCSNode &in);

	// Specialize Constructor
	PCSNode(PCSNode * const inParent, PCSNode * const inChild, PCSNode * const inNextSibling, PCSNode * const inPrevSibling, const char * const inName);
	PCSNode(const char * const inName);

	// destructor
	virtual ~PCSNode();

	// assignment operator
	PCSNode &operator = (const PCSNode &in);

	// accessors
	void setParent(PCSNode * const in);
	void setChild(PCSNode * const in);
	void setNextSibling(PCSNode * const in);
	void setPrevSibling(PCSNode * const in);
	void setForward(PCSNode * const in);
	void setReverse(PCSNode * const in);

	PCSNode *getParent(void) const;
	PCSNode *getChild(void) const;
	PCSNode *getNextSibling(void) const;
	PCSNode *getPrevSibling(void) const;
	PCSNode *getForward(void) const;
	PCSNode *getReverse(void) const;

	// name
	PCSNodeReturnCode setName(const char * const inName);
	PCSNodeReturnCode getName(char * const outBuffer, unsigned int sizeOutBuffer) const;

	// print
	void printNode() const;
	void printChildren() const;
	void printSiblings() const;

	// get number of children/siblings
	int getNumSiblings() const;
	int getNumChildren() const;

private:
	PCSNode *parent;
	PCSNode *child;
	PCSNode *nextSibling;
	PCSNode *prevSibling;
	PCSNode *forward;
	PCSNode *reverse; 

	char     name[PCSNODE_NAME_SIZE];
};


#endif