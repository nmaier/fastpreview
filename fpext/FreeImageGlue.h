#ifndef FREEIMAGEGLUE_H
#define FREEIMAGEGLUE_H

#include <FreeImagePlus.h>

namespace FreeImage {

class Glue {
public:
	Glue() {
		FreeImage_Initialise();
	}
	~Glue() {
		FreeImage_DeInitialise();
	}
};

}; //namespace


#endif