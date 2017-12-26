/*
 * string-utils.cc
 *
 *  Created on: 2 de abr de 2017
 *      Author: gabriel
 */

#include "string-utils.h"
#include <boost/algorithm/string.hpp>
#include<boost/tokenizer.hpp>


std::string trim(std::string& s) {

	return boost::trim_copy(s);

}


std::string rtrim(std::string& s) {

	return boost::trim_right_copy(s);

}


std::string toLowerCase(const std::string& s) {

	return boost::to_lower_copy(s);

}
