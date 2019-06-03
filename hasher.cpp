#include <cassert>
#include <istream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>

#include <boost/algorithm/hex.hpp>
#include <boost/filesystem.hpp>

#include <hasher.hpp>

#ifndef PS_USE_BCRYPT_WIN

#include <ext/picosha2.h>

ps_sha_t
_fname_checksum(const boost::filesystem::path &file)
{
	unsigned char sha[picosha2::k_digest_size] = {};
	std::ifstream ifst = boost::filesystem::ifstream(file, std::ios_base::in | std::ios_base::binary);
	picosha2::hash256(ifst, std::begin(sha), std::end(sha));
	return std::string(std::begin(sha), std::end(sha));
}

#else /* PS_USE_BCRYPT_WIN */

// http://kirkshoop.blogspot.com/2011/09/ntstatus.html
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <winternl.h>
#include <ntstatus.h>
#include <bcrypt.h>

using ps_crypt_t = std::tuple<std::shared_ptr<BCRYPT_ALG_HANDLE>, std::shared_ptr<BCRYPT_HASH_HANDLE>, std::shared_ptr<unsigned char>, std::shared_ptr<unsigned char>, size_t>;

inline void
_sp_del_bcryptalghandle(BCRYPT_ALG_HANDLE *p)
{
	if (p && *p)
		BCryptCloseAlgorithmProvider(*p, 0);
	delete p;
}

inline void
_sp_del_bcrypthashhandle(BCRYPT_HASH_HANDLE *p)
{
	if (p && *p)
		BCryptDestroyHash(*p);
	delete p;
}

inline void
_sp_del_uchar_arr(unsigned char *p)
{
	delete[] p;
}

inline ps_crypt_t
_mkcrypt()
{
	std::shared_ptr<BCRYPT_ALG_HANDLE> bah(new BCRYPT_ALG_HANDLE(nullptr), _sp_del_bcryptalghandle);
	std::shared_ptr<BCRYPT_HASH_HANDLE> bhh(new BCRYPT_HASH_HANDLE(nullptr), _sp_del_bcrypthashhandle);
	DWORD cbHashObject = 0, cbHash = 0;
	ULONG cbData = 0;
	if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(bah.get(), BCRYPT_SHA256_ALGORITHM, nullptr, 0)))
		throw std::runtime_error("");
	if (!NT_SUCCESS(BCryptGetProperty(*bah, BCRYPT_OBJECT_LENGTH, (PUCHAR)&cbHashObject, sizeof cbHashObject, &cbData, 0)))
		throw std::runtime_error("");
	if (!NT_SUCCESS(BCryptGetProperty(*bah, BCRYPT_HASH_LENGTH, (PUCHAR)& cbHash, sizeof cbHash, &cbData, 0)))
		throw std::runtime_error("");
	assert(cbHashObject > 0 && cbHashObject < 8096 && cbHash > 0 && cbHash < 8096);
	std::shared_ptr<unsigned char> pbHashObject(new unsigned char[cbHashObject](), _sp_del_uchar_arr);
	std::shared_ptr<unsigned char> pbHash(new unsigned char[cbHash](), _sp_del_uchar_arr);
	if (!NT_SUCCESS(BCryptCreateHash(*bah, bhh.get(), pbHashObject.get(), cbHashObject, nullptr, 0, 0)))
		throw std::runtime_error("");
	return std::make_tuple(bah, bhh, pbHashObject, pbHash, cbHash);
}

inline void
_hhhashdata(const ps_crypt_t &cryp, char *buf, size_t buf_len)
{
	if (!NT_SUCCESS(BCryptHashData(*std::get<1>(cryp), (PUCHAR) buf, (ULONG) buf_len, 0)))
		throw std::runtime_error("");
}

inline std::string
_hhfinish(const ps_crypt_t &cryp)
{
	if (!NT_SUCCESS(BCryptFinishHash(*std::get<1>(cryp), (PUCHAR) std::get<3>(cryp).get(), (ULONG) std::get<4>(cryp), 0)))
		throw std::runtime_error("");
	std::string hash((char *)std::get<3>(cryp).get(), std::get<4>(cryp));
	return hash;
}

inline void
_hhcrypt(const ps_crypt_t &cryp)
{
	const char ww[] = "hello";
	if (!NT_SUCCESS(BCryptHashData(*std::get<1>(cryp), (PUCHAR) ww, sizeof ww, 0)))
		throw std::runtime_error("");
	if (!NT_SUCCESS(BCryptFinishHash(*std::get<1>(cryp), (PUCHAR) std::get<3>(cryp).get(), (ULONG) std::get<4>(cryp), 0)))
		throw std::runtime_error("");
}

inline std::string
_fname_checksum_bin(const boost::filesystem::path &file)
{
	bool pending_end = false;
	char buf[16 * 4096] = {};
	std::ifstream ifst = boost::filesystem::ifstream(file, std::ios_base::in | std::ios_base::binary);
	ps_crypt_t cryp(_mkcrypt());
	do {
		if ((pending_end = !ifst.read(buf, sizeof buf)); ifst.gcount())
			_hhhashdata(cryp, buf, ifst.gcount());
	} while (!pending_end);
	if (!ifst.eof())
		throw std::runtime_error("");
	return _hhfinish(cryp);
}

ps_sha_t
_fname_checksum(const boost::filesystem::path &file)
{
	return boost::algorithm::hex(_fname_checksum_bin(file));
}

#endif /* PS_USE_BCRYPT_WIN */
