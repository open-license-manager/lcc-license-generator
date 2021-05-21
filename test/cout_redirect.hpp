/*
 * cout_redirect.hpp
 *
 *  Created on: Dec 9, 2019
 *      Author: devel
 */

#ifndef TEST_COUT_REDIRECT_HPP_
#define TEST_COUT_REDIRECT_HPP_

namespace license {
namespace test {
struct cout_redirect {
	cout_redirect(std::streambuf* new_buffer) : old(std::cout.rdbuf(new_buffer)) {}
	~cout_redirect() { std::cout.rdbuf(old); }

private:
	std::streambuf* old;
};
}  // namespace test
}  // namespace license

#endif /* TEST_COUT_REDIRECT_HPP_ */
