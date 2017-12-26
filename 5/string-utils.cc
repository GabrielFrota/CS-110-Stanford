/*
 * string-utils.cc
 *
 *  Created on: 2 de abr de 2017
 *      Author: gabriel
 */

#include "string-utils.h"
#include <boost/algorithm/string.hpp>

std::string trim(std::string& s) {

	return boost::trim_copy(s);

}



