#include "./OpenMPD_Engine/src/Buffer.h"
#include "./OpenMPD_Engine/src/BufferManager.h"

int OpenMPD::Buffer::instanceCounter=0;


bool OpenMPD::Buffer::split(cl_uint size, OpenMPD::Buffer** first, OpenMPD::Buffer** second) {
	if (size > this->size())
		return false; //Cannot split
	*first = new OpenMPD::Buffer(this->start, this->start + size, BufferManager::instance().getValidID());
	*second = new OpenMPD::Buffer(this->start + size, this->end, BufferManager::instance().getValidID());
	return true;
}

/**
	Creates a new buffer as a result of merging two existing ADJACENT buffers. 
	Fails if buffers are not adjacent. 
	Does not destroy the two buffers merged.
*/
OpenMPD::Buffer* OpenMPD::Buffer::merge(OpenMPD::Buffer* a, OpenMPD::Buffer* b) {
	//1. Check conditions
	OpenMPD::Buffer *pre, *post;
	if (a->preAdjacent(*b)) {
		pre = a; post = b;
	}
	else if (a->postAdjacent(*b)) {
		pre = b; post = a;
	}
	else {
		OpenMPD::printWarning_OpenMPD("BufferManager::Invalid merge command. Gaps are not adjacent (ignoring command).");
		return NULL; //Cannot merge.
	}
	//2. Merge
	return new OpenMPD::Buffer(pre->start, post->end, OpenMPD::BufferManager::instance().getValidID());
}

