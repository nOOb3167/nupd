#ifndef _PSCON_HPP_
#define _PSCON_HPP_

#include <cassert>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include <stdexcept>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

using tcp = ::boost::asio::ip::tcp;
namespace http = ::boost::beast::http;

using res_t = ::http::response<http::string_body>;

class con_tag_ratio_t {};

class ConProgress
{
public:
	inline void onRequest(const std::string &path, const std::string &data)
	{
		std::lock_guard<std::mutex> l(m_mtx);
	}

	std::mutex m_mtx;
};

class PsCon
{
public:
	inline virtual ~PsCon() {};
	inline virtual res_t req(const std::string &path, const std::string &data) = 0;
	
	ConProgress m_prog;
};

class PsConNet : public PsCon
{
public:
	inline PsConNet(const std::string &host, const std::string &port, const std::string &host_http_rootpath) :
		PsCon(),
		m_host(host),
		m_port(port),
		m_host_http(host + ":" + port),
		m_host_http_rootpath(host_http_rootpath),
		m_ioc(),
		m_resolver(m_ioc),
		m_resolver_r(m_resolver.resolve(host, port)),
		m_socket(new tcp::socket(m_ioc))
	{
		boost::asio::connect(*m_socket, m_resolver_r.begin(), m_resolver_r.end());
	};

	inline virtual ~PsConNet() override
	{
		m_socket->shutdown(tcp::socket::shutdown_both);
	}

	inline void
	_reconnect()
	{
		m_socket.reset(new tcp::socket(m_ioc));
		boost::asio::connect(*m_socket, m_resolver_r.begin(), m_resolver_r.end());
	}

	inline res_t
	req_(const http::verb &verb, const std::string &path, const std::string &data)
	{
		http::request<http::string_body> req(verb, m_host_http_rootpath + path, 11);
		req.set(http::field::host, m_host_http);
		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
		http::write(*m_socket, req);
		boost::beast::flat_buffer buffer;
		http::response<http::string_body> res;
		http::read(*m_socket, buffer, res);
		// https://github.com/boostorg/beast/issues/927
		//   Repeated calls to an URL (repeated http::write calls without remaking the socket)
		//     - needs http::response::keep_alive() true
		//     - (for which http::response::version() must be 11 (HTTP 1.1))
		// https://www.reddit.com/r/flask/comments/634i5u/make_flask_return_header_response_with_http11/
		//   You can't. Flask's dev server does not implement the HTTP 1.1 spec
		//     - flask does not support HTTP 1.1, remake socket if necessary
		if (!res.keep_alive())
			_reconnect();
		return res;
	}

	inline virtual res_t
	req(const std::string &path, const std::string &data) override
	{
		m_prog.onRequest(path, data);
		res_t res = req_(http::verb::get, path, data);
		if (res.result_int() != 200)
			throw std::runtime_error("");
		return res;
	}

	std::string m_host;
	std::string m_port;
	std::string m_host_http;
	std::string m_host_http_rootpath;
	boost::asio::io_context m_ioc;
	tcp::resolver m_resolver;
	tcp::resolver::results_type m_resolver_r;
	std::shared_ptr<tcp::socket> m_socket;
};

class PsConFs : public PsCon
{
public:
	inline PsConFs(const std::string &dir) :
		PsCon(),
		m_dir(dir)
	{
		if (!boost::filesystem::is_directory(m_dir))
			throw std::runtime_error("");
	};

	inline virtual ~PsConFs() override
	{
	}

	inline virtual res_t
	req(const std::string &path, const std::string &data) override
	{
		m_prog.onRequest(path, data);
		return res_t(boost::beast::http::status::ok, 11, std::string());
	}

	boost::filesystem::path m_dir;
};

#endif /* _PSCON_HPP_ */
