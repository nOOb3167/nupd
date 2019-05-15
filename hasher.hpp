#ifndef _HASHER_HPP_
#define _HASHER_HPP_

#include <string>

#include <boost/filesystem.hpp>

using ps_sha_t = std::string;

ps_sha_t _fname_checksum(const boost::filesystem::path &file);

#endif /* _HASHER_HPP_ */
