#ifndef _PSCON_HPP_
#define _PSCON_HPP_

#include <cassert>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <stdexcept>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/thread/barrier.hpp>

using tcp = ::boost::asio::ip::tcp;
namespace http = ::boost::beast::http;

using res_t = ::http::response<http::string_body>;

class con_tag_ratio_t {};

class XRunInThread
{
public:
	inline XRunInThread(const std::function<void()> &f) :
		m_f(f),
		m_e(),
		m_t(std::bind(&XRunInThread::_run, this))
	{}

	inline virtual ~XRunInThread()
	{
		if (m_t.joinable())
			m_t.join();
		if (m_e)
			std::rethrow_exception(m_e);
	}

	inline void
	_run()
	{
		try {
			m_f();
		}
		catch (...) {
			m_e = std::current_exception();
		}
	}

	std::function<void()> m_f;
	std::exception_ptr m_e;
	std::thread m_t;
};

inline std::string
_readfile(const boost::filesystem::path &path)
{
	boost::filesystem::ifstream ifst = boost::filesystem::ifstream(path, std::ios_base::in | std::ios_base::binary);
	std::stringstream ss;
	ss << ifst.rdbuf();
	// ss fail conditions: error OR file is empty
	// https://en.cppreference.com/w/cpp/io/basic_ostream/operator_ltlt
	// https://en.cppreference.com/w/cpp/io/basic_streambuf/pubseekoff
	//   If no characters were inserted, executes setstate(failbit). If an exception was thrown while extracting, sets failbit
	//   "no characters were inserted" applies on an empty file
	// to distinguish error from empty file
	// (note istream::rdbuf::pubseekoff as in istream::{seekg,tellg} tellg returns -1 on a fail condition)
	if (!ss.good() && ifst.rdbuf()->pubseekoff(0, std::ios_base::end, std::ios_base::in) != 0)
		throw std::runtime_error("");
	if (!ifst.good())
		throw std::runtime_error("");
	return ss.str();
}

inline std::string
_read_oneshot_timeout(boost::asio::io_service &serv, tcp::socket &sock, size_t timo_ms)
{
	boost::asio::streambuf stre;
	boost::asio::deadline_timer timo(serv);

	size_t nread = 0;

	std::function<void(const boost::system::error_code &)> wc = [&sock](const boost::system::error_code &ec) {
		if (ec)
			return;
		sock.cancel();
	};
	std::function<void(size_t, const boost::system::error_code &)> rc = [&timo, &nread](size_t bytes_transferred, const boost::system::error_code &ec) {
		if (ec && ec != boost::asio::error::operation_aborted || !bytes_transferred)
			return;
		timo.cancel();
		nread = bytes_transferred;
	};

	timo.expires_from_now(boost::posix_time::milliseconds(timo_ms));
	timo.async_wait(boost::bind(wc, boost::asio::placeholders::error));
	boost::asio::async_read(sock, stre, boost::asio::transfer_all(), boost::bind(rc, boost::asio::placeholders::bytes_transferred, boost::asio::placeholders::error));

	serv.run();
	serv.reset();

	const auto &bufs = stre.data();
	return std::string(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + nread);
}

inline std::string
_accept_oneshot_http(const std::string &strport, size_t timo_ms, boost::barrier &bar_listen)
{
	boost::asio::io_service serv;
	tcp::endpoint endp(tcp::v4(), (unsigned short)(std::stoi(strport)));
	tcp::acceptor acce(serv, endp);
	acce.listen();

	bar_listen.wait();

	tcp::socket sock = acce.accept();

	std::string resp = _read_oneshot_timeout(serv, sock, timo_ms);

	boost::asio::write(sock, boost::asio::buffer("HTTP/1.1 200 OK\r\n\r\n"), boost::asio::transfer_all());

	sock.close();

	return resp;
}

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

public:
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

	inline static std::string
	_joinpath(const std::string &rootpath, const std::string &path)
	{
		boost::cmatch what;
		if (!boost::regex_match(rootpath.c_str(), what, boost::regex("(/([[:word:]]+/)*)?")))
			throw std::runtime_error("");
		if (path.size() && path.at(0) == '/')
			throw std::runtime_error("");
		return rootpath + path;
	}

	inline res_t
	req_(const http::verb &verb, const std::string &path, const std::string &data)
	{
		http::request<http::string_body> req(verb, _joinpath(m_host_http_rootpath, path), 11);
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
	inline PsConFs(const boost::filesystem::path &rootdir) :
		PsCon(),
		m_rootdir(rootdir)
	{
		if (!boost::filesystem::is_directory(m_rootdir))
			throw std::runtime_error("");
	};

	inline virtual ~PsConFs() override
	{
	}

	inline virtual res_t
	req(const std::string &path, const std::string &data) override
	{
		m_prog.onRequest(path, data);
		return res_t(boost::beast::http::status::ok, 11, _readfile(m_rootdir / path));
	}

	boost::filesystem::path m_rootdir;
};

#endif /* _PSCON_HPP_ */
