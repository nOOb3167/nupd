#define BOOST_TEST_MODULE nupd
#include <boost/test/included/unit_test.hpp>

#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <istream>
#include <map>
#include <set>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <hasher.hpp>
#include <pscon.hpp>

#include <boost/filesystem.hpp>

// boost::filesystem::{weakly_}canonical : weakly does not require existence

using fpt_t = std::tuple<boost::filesystem::path, std::string>;
using fpt3_t = std::tuple<std::vector<fpt_t>, std::vector<fpt_t>, std::vector<fpt_t> >;
using ps_sha_t = std::string;

const char ps_uniq_path_pattern[] = "pstmp%%%%-%%%%-%%%%-%%%%";

template<typename T, typename U>
class ItPair
{
public:
	inline constexpr static bool IsConst = std::is_const<T>::value && std::is_const<U>::value;

	class It
	{
	public:
		using valtyp = std::tuple<const typename T::value_type &, const typename U::value_type &>;
		using itatyp = typename std::conditional<IsConst, typename T::const_iterator, typename T::iterator>::type;
		using itbtyp = typename std::conditional<IsConst, typename U::const_iterator, typename U::iterator>::type;

		inline It(itatyp &ia, itbtyp &ib) : m_tup(std::make_tuple(ia, ib)) {}
		inline It operator++() { ++std::get<0>(m_tup); ++std::get<1>(m_tup); return *this; }
		inline bool operator!=(const It &other) const { return m_tup != other.m_tup; }
		inline const valtyp operator*() const { return valtyp(*std::get<0>(m_tup), *std::get<1>(m_tup)); }

		std::tuple<itatyp, itbtyp> m_tup;
	};

	inline ItPair(T &a, U &b) : m_a(a), m_b(b)
	{
		if (a.size() != b.size())
			throw std::out_of_range("");
	}

	inline It
	begin() const
	{
		return It(m_a.begin(), m_b.begin());
	}

	inline It
	end() const
	{
		return It(m_a.end(), m_b.end());
	}

	T &m_a;
	U &m_b;
};

class NupdD
{
public:
	using dd_t = std::map<boost::filesystem::path, NupdD>;

	inline NupdD(const ps_sha_t &a, const ps_sha_t &b) : m_a(a), m_b(b) {}
	inline NupdD() = default;
	inline NupdD(const NupdD &) = default;

	inline static dd_t mk(
		const std::vector<boost::filesystem::path> &beg_fils,
		const std::vector<ps_sha_t> &beg_sums,
		const std::vector<boost::filesystem::path> &goal_fils,
		const std::vector<ps_sha_t> &goal_sums)
	{
		dd_t dd;
		assert(beg_fils.size() == beg_sums.size());
		assert(goal_fils.size() == goal_sums.size());
		for (size_t i = 0; i < goal_fils.size(); i++)
			dd[goal_fils[i]].m_a = goal_sums[i];
		for (size_t i = 0; i < beg_fils.size(); i++)
			dd[beg_fils[i]].m_b = beg_sums[i];
		return dd;
	}

	inline static void
	xform_AB_AN__XX_NB(NupdD &fr, NupdD &to)
	{
		NupdD fr_(fr.m_a, ps_sha_t());
		NupdD to_(ps_sha_t(), fr.m_b);
		fr = fr_;
		to = to_;
	}

	inline static void
	xform_AN_AA(NupdD &v)
	{
		v = NupdD(v.m_a, v.m_a);
	}

	ps_sha_t m_a;
	ps_sha_t m_b;
};

using nupdd_t = std::map<boost::filesystem::path, NupdD>;

inline std::vector<boost::filesystem::path>
_fnames_rec_sorted(const boost::filesystem::path &dirp)
{
	std::vector<boost::filesystem::path> fils;
	for (auto &it = boost::filesystem::recursive_directory_iterator(dirp); it != boost::filesystem::recursive_directory_iterator(); ++it)
		if (const boost::filesystem::path &f = it->path(); boost::filesystem::is_regular_file(f))
			fils.push_back(f);
	std::sort(fils.begin(), fils.end());
	return fils;
}

inline std::vector<ps_sha_t>
_fnames_checksum(const std::vector<boost::filesystem::path> &fils)
{
	std::vector<ps_sha_t> shas;
	for (size_t i = 0; i < fils.size(); i++)
		shas.push_back(_fname_checksum(fils[i]));
	return shas;
}

inline std::tuple<std::vector<boost::filesystem::path>, std::vector<ps_sha_t> >
_dir_checksum(const boost::filesystem::path &dirp)
{
	std::vector<boost::filesystem::path> fils_ = _fnames_rec_sorted(dirp);
	std::vector<ps_sha_t> sums = _fnames_checksum(fils_);
	std::vector<boost::filesystem::path> fils;
	for (size_t i = 0; i < fils_.size(); i++)
		fils.push_back(boost::filesystem::relative(fils_[i], dirp));
	return std::make_tuple(fils, sums);
}

inline std::vector<ps_sha_t>
_missing_checksum(const std::vector<ps_sha_t> &xold, const std::vector<ps_sha_t> &xnew)
{
	std::set<ps_sha_t> sold;
	for (auto &v : xold)
		sold.insert(v);
	std::vector<ps_sha_t> miss;
	for (auto &v : xnew)
		if (sold.find(v) == sold.end())
			miss.push_back(v);
	return miss;
}

inline std::tuple<boost::filesystem::path, boost::filesystem::path>
_tmp_copy_tempname(const boost::filesystem::path &src, const boost::filesystem::path &dstroot)
{
	boost::filesystem::path dstp = dstroot / boost::filesystem::unique_path();
	boost::filesystem::copy_file(src, dstp, boost::filesystem::copy_option::fail_if_exists);
	return std::make_tuple(dstroot, boost::filesystem::relative(dstp, dstroot));
}

inline std::tuple<boost::filesystem::path, boost::filesystem::path>
_tmp_move_tempname(const boost::filesystem::path &src, const boost::filesystem::path &dstroot)
{
	boost::filesystem::path dstp = dstroot / boost::filesystem::unique_path();
	boost::filesystem::rename(src, dstp);
	return std::make_tuple(dstroot, boost::filesystem::relative(dstp, dstroot));
}

inline void
_del_last_if_file(const boost::filesystem::path &path)
{
	for (auto &cano = boost::filesystem::weakly_canonical(path); cano.has_parent_path(); cano = cano.parent_path())
		if (boost::filesystem::exists(cano)) {
			if (boost::filesystem::is_regular_file(cano))
				boost::filesystem::remove(cano);
			return;
		}
}

inline void
_tmp_copy_force_makedst(const boost::filesystem::path &src, const boost::filesystem::path &dst_)
{
	const auto &dst = boost::filesystem::weakly_canonical(dst_);
	assert(dst.has_parent_path());
	_del_last_if_file(dst);
	boost::filesystem::create_directories(dst.parent_path());
	if (boost::filesystem::exists(dst))
		boost::filesystem::rename(dst, boost::filesystem::temp_directory_path() / boost::filesystem::unique_path(ps_uniq_path_pattern));
	boost::filesystem::copy_file(src, dst, boost::filesystem::copy_option::fail_if_exists);
}

inline std::tuple<std::vector<boost::filesystem::path>, std::vector<ps_sha_t> >
_tmp_fakedl(
	const boost::filesystem::path &srcroot,
	const boost::filesystem::path &dstroot,
	const std::vector<ps_sha_t> &dlsha,
	const std::vector<boost::filesystem::path> &aux_fils,
	const std::vector<ps_sha_t> &aux_sums)
{
	std::map<ps_sha_t, boost::filesystem::path> dsf;
	for (const auto &[k, v] : ItPair(aux_fils, aux_sums))
		dsf[v] = k;

	std::vector<boost::filesystem::path> fils;
	for (const auto &v : dlsha)
		fils.push_back(std::get<1>(_tmp_copy_tempname(srcroot / dsf.at(v), dstroot)));

	return std::make_tuple(fils, dlsha);
}

inline int
_main(const boost::filesystem::path &ourroot, const boost::filesystem::path &theroot)
{
	const auto &[goal_fils, goal_sums] = _dir_checksum(theroot);
	auto &[beg_fils, beg_sums] = _dir_checksum(ourroot);

	std::vector<ps_sha_t> miss_sums = _missing_checksum(beg_sums, goal_sums);

	if (miss_sums.size()) {
		auto &[dl_fils, dl_sums] = _tmp_fakedl(theroot, ourroot, miss_sums, goal_fils, goal_sums);
		std::copy(dl_fils.begin(), dl_fils.end(), std::back_inserter(beg_fils));
		std::copy(dl_sums.begin(), dl_sums.end(), std::back_inserter(beg_sums));
	}
		
	nupdd_t dd = NupdD::mk(beg_fils, beg_sums, goal_fils, goal_sums);

	std::vector<std::tuple<boost::filesystem::path, boost::filesystem::path> > work;

	for (const auto &[k, v] : dd)
		if (v.m_a.size() && v.m_b.size() && v.m_a != v.m_b)
			work.push_back(std::make_tuple(k, std::get<1>(_tmp_move_tempname(ourroot / k, ourroot))));
	for (const auto &[k, rel] : work)
		NupdD::xform_AB_AN__XX_NB(dd[k], dd[rel]);

	std::map<ps_sha_t, boost::filesystem::path> dsf;
	for (const auto &[k, v] : ItPair(beg_fils, beg_sums))
		dsf[v] = k;

	std::vector<boost::filesystem::path> work2;
	for (const auto &[k, v] : dd)
		if (v.m_a.size() && !v.m_b.size()) {
			_tmp_copy_force_makedst(ourroot / dsf.at(v.m_a), ourroot / k);
			work2.push_back(k);
		}
	for (const auto &k : work2)
		NupdD::xform_AN_AA(dd.at(k));

	for (const auto &[k, v] : ItPair(goal_fils, goal_sums))
		assert(_fname_checksum(ourroot / k) == v);

	return EXIT_SUCCESS;
}

class TmpDirX
{
public:
	inline TmpDirX() :
		m_d(boost::filesystem::temp_directory_path() / boost::filesystem::unique_path(ps_uniq_path_pattern))
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

	TmpDirX m_tmpd_our;
	TmpDirX m_tmpd_the;
	std::vector<fpt_t> m_fpt_our;
	std::vector<fpt_t> m_fpt_the;
	std::vector<fpt_t> m_fpt_fin;
};

BOOST_AUTO_TEST_SUITE(nupd_suite);

BOOST_AUTO_TEST_CASE(nupd_main0)
{
	TmpDirFixture w(
		{ {"a.txt", "a"} },
		{ {"a.txt", "a"} },
		{ {"a.txt", "a"} }
	);
	_main(w.m_tmpd_our.m_d, w.m_tmpd_the.m_d);
}

BOOST_AUTO_TEST_CASE(nupd_main1)
{
	TmpDirFixture w(
		{},
		{ {"a.txt", "a"} },
		{ {"a.txt", "a"} }
	);
	_main(w.m_tmpd_our.m_d, w.m_tmpd_the.m_d);
}

BOOST_AUTO_TEST_CASE(nupd_main2)
{
	TmpDirFixture w(
		{ {"a.txt", "a"} },
		{ {"a.txt", "b"} },
		{ {"a.txt", "b"} }
	);
	_main(w.m_tmpd_our.m_d, w.m_tmpd_the.m_d);
}

BOOST_AUTO_TEST_CASE(nupd_main3)
{
	TmpDirFixture w(
		{ {"a0.txt", "c"}, {"a1.txt", "c"} },
		{ {"a.txt", "b"} },
		{ {"a.txt", "b"}, {"a1.txt", "c" } }
	);
	_main(w.m_tmpd_our.m_d, w.m_tmpd_the.m_d);
}

BOOST_AUTO_TEST_CASE(nupd_main4)
{
	TmpDirFixture w(
		{ {"a0.txt", "c"} },
		{ {"a.txt", "b"}, {"b.txt", "c"} },
		{ {"a0.txt", "c"}, {"a.txt", "b"}, {"b.txt", "c"} }
	);
	_main(w.m_tmpd_our.m_d, w.m_tmpd_the.m_d);
}

BOOST_AUTO_TEST_SUITE_END();
