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
	cout_redirect(std::streambuf* new_buffer) : old_cout(std::cout.rdbuf(new_buffer)), old_cerr(std::cerr.rdbuf(new_buffer)) {}

	~cout_redirect() { std::cout.rdbuf(old_cout); std::cerr.rdbuf(old_cerr); }

private:
	std::streambuf* old_cout;
	std::streambuf* old_cerr;
};

}  // namespace test
}  // namespace license

#endif /* TEST_COUT_REDIRECT_HPP_ */
