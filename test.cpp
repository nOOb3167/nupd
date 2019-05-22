#define BOOST_TEST_MODULE nupd
#include <boost/test/included/unit_test.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <boost/algorithm/string/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/thread/barrier.hpp>

#include <pscon.hpp>
#include <psnupd.hpp>

using fpt_t = std::tuple<boost::filesystem::path, std::string>;
using fpt3_t = std::tuple<std::vector<fpt_t>, std::vector<fpt_t>, std::vector<fpt_t> >;

inline std::vector<std::string>
_re_getline(const std::string &str)
{
	// https://www.boost.org/doc/libs/1_31_0/libs/regex/doc/syntax.html
	//   Expression  Text  POSIX leftmost longest match  ECMAScript depth first search match
	//   a|ab        xaby  "ab"                          "a"
	// POSIX is boost::regex::extended, ECMAScript boost:regex::normal is the default
	std::vector<std::string> line;
	boost::algorithm::split_regex(line, str, boost::regex("\n|\r|(\r\n)", boost::regex::extended));
	return line;
}

inline bool
_re_match(const std::string &str, const char *regx)
{
	return boost::regex_match(str.c_str(), boost::cmatch(), boost::regex(regx), boost::match_default);
}

class TmpDirX
{
	inline const static char uniq_path_pattern[] = "pstmp%%%%-%%%%-%%%%-%%%%";
public:
	inline TmpDirX() :
		m_d(boost::filesystem::temp_directory_path() / boost::filesystem::unique_path(uniq_path_pattern))
	{
		boost::filesystem::create_directories(m_d);
	}

	inline ~TmpDirX()
	{
		if (boost::filesystem::is_directory(m_d))
			boost::filesystem::remove_all(m_d);
	}

	boost::filesystem::path m_d;
};

class TmpDirFixture
{
public:
	inline TmpDirFixture(const fpt3_t &fpt) :
		TmpDirFixture(std::get<0>(fpt), std::get<1>(fpt), std::get<2>(fpt))
	{}

	inline TmpDirFixture(
		const std::vector<fpt_t> &fpt_our,
		const std::vector<fpt_t> &fpt_the,
		const std::vector<fpt_t> &fpt_fin
	) :
		m_tmpd_our(),
		m_tmpd_the(),
		m_fpt_our(fpt_our),
		m_fpt_the(fpt_the),
		m_fpt_fin(fpt_fin)
	{
		_filldir(m_tmpd_our.m_d, fpt_our);
		_filldir(m_tmpd_the.m_d, fpt_the);
	}

	inline virtual ~TmpDirFixture()
	{
		for (const auto &[k, v] : m_fpt_fin)
			BOOST_REQUIRE(_readfile(m_tmpd_our.m_d / k) == v);
	}

	inline static void
	_filldir(const boost::filesystem::path &tmpd, const std::vector<fpt_t> &fpt)
	{
		for (auto &[k, v] : fpt) {
			BOOST_REQUIRE(k.is_relative() && k == k.lexically_normal() && (tmpd / k).has_parent_path());
			boost::filesystem::create_directories((tmpd / k).parent_path());
			if (!boost::filesystem::ofstream((tmpd / k), std::ios_base::out | std::ios_base::binary).write(v.data(), v.size()).good())
				throw std::runtime_error("");
		}
	}

	inline static std::string
	_readfile(const boost::filesystem::path &path)
	{
		std::stringstream buf;
		buf << boost::filesystem::ifstream(path, std::ios_base::in | std::ios_base::binary).rdbuf();
		return buf.str();
	}

	inline TmpDirFixture &
	_mklistfile()
	{
		std::vector<fpt_t> fpt_the(m_fpt_the);
	}

	TmpDirX m_tmpd_our;
	TmpDirX m_tmpd_the;
	std::vector<fpt_t> m_fpt_our;
	std::vector<fpt_t> m_fpt_the;
	std::vector<fpt_t> m_fpt_fin;
};

BOOST_AUTO_TEST_SUITE(nupd_suite);

BOOST_AUTO_TEST_CASE(nupd_mklistfile)
{
	TmpDirFixture w(
		{ {"a.txt", "a"} },
		{},
		{}
	);
	BOOST_REQUIRE(_dir_mklistfile(w.m_tmpd_our.m_d) == "\"a.txt\" CA978112CA1BBDCAFAC231B39A23DC4DA786EFF8147C4E72B9807785AFEE48BB\n");
}

BOOST_AUTO_TEST_CASE(nupd_main0)
{
	TmpDirFixture w(
		{ {"a.txt", "a"} },
		{ {"a.txt", "a"} },
		{ {"a.txt", "a"} }
	);
	PsConFs psco(w.m_tmpd_the.m_d);
	_main(w.m_tmpd_our.m_d, w.m_tmpd_the.m_d, psco);
}

BOOST_AUTO_TEST_CASE(nupd_main1)
{
	TmpDirFixture w(
		{},
		{ {"a.txt", "a"} },
		{ {"a.txt", "a"} }
	);
	PsConFs psco(w.m_tmpd_the.m_d);
	_main(w.m_tmpd_our.m_d, w.m_tmpd_the.m_d, psco);
}

BOOST_AUTO_TEST_CASE(nupd_main2)
{
	TmpDirFixture w(
		{ {"a.txt", "a"} },
		{ {"a.txt", "b"} },
		{ {"a.txt", "b"} }
	);
	PsConFs psco(w.m_tmpd_the.m_d);
	_main(w.m_tmpd_our.m_d, w.m_tmpd_the.m_d, psco);
}

BOOST_AUTO_TEST_CASE(nupd_main3)
{
	TmpDirFixture w(
		{ {"a0.txt", "c"}, {"a1.txt", "c"} },
		{ {"a.txt", "b"} },
		{ {"a.txt", "b"}, {"a1.txt", "c" } }
	);
	PsConFs psco(w.m_tmpd_the.m_d);
	_main(w.m_tmpd_our.m_d, w.m_tmpd_the.m_d, psco);
}

BOOST_AUTO_TEST_CASE(nupd_main4)
{
	TmpDirFixture w(
		{ {"a0.txt", "c"} },
		{ {"a.txt", "b"}, {"b.txt", "c"} },
		{ {"a0.txt", "c"}, {"a.txt", "b"}, {"b.txt", "c"} }
	);
	PsConFs psco(w.m_tmpd_the.m_d);
	_main(w.m_tmpd_our.m_d, w.m_tmpd_the.m_d, psco);
}

BOOST_AUTO_TEST_CASE(nupd_con_joinpath)
{
	BOOST_CHECK_NO_THROW(PsConNet::_joinpath("", ""));
	BOOST_CHECK_NO_THROW(PsConNet::_joinpath("/", ""));
	BOOST_CHECK_NO_THROW(PsConNet::_joinpath("/abc/", ""));
	BOOST_CHECK_NO_THROW(PsConNet::_joinpath("/abc/def/", ""));
	BOOST_CHECK_THROW(PsConNet::_joinpath("/abc", ""), std::runtime_error);
	BOOST_CHECK_THROW(PsConNet::_joinpath("/abc/def", ""), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(nupd_con0)
{
	TmpDirFixture w(
		{ {"a.txt", ""}, {"b.txt", "b"}, {"d/e/c.txt", "c"} },
		{},
		{}
	);
	PsConFs c(w.m_tmpd_our.m_d);
	c.req("a.txt", "").body() == "";
	c.req("b.txt", "").body() == "b";
	c.req("d/e/c.txt", "").body() == "c";
}

BOOST_AUTO_TEST_CASE(nupd_con1)
{
	boost::barrier barr(2);
	TmpDirFixture w(
		{},
		{},
		{}
	);
	XRunInThread r([&]() {
		const auto &str = _accept_oneshot_http("9865", 1000, barr);
		std::vector<std::string> line = _re_getline(str);
		BOOST_CHECK(_re_match(line.at(0), "GET /test/a.txt .*"));
		BOOST_CHECK(_re_match(line.at(1), "Host: .*"));
		BOOST_CHECK(_re_match(line.at(2), "User-Agent: .*"));
	});
	barr.wait();
	PsConNet c("localhost", "9865", "/test/");
	c.req("a.txt", "").body();
}

BOOST_AUTO_TEST_SUITE_END();
